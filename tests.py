import re
import csv
from datetime import datetime, timedelta
from collections import defaultdict

# Parámetros de configuración
mes = 1
año = 25
input_filtered_file = f"filtrado_mes_{mes}_año_{año}.txt"
output_csv_file = f"csv_filtrado_mes_{mes}_año_{año}.csv"
tabla_filtrada = f"tabla_filtrado_mes_{mes}_año_{año}.csv"

# Función para convertir a datetime
def convertir_datetime(fecha_str, hora_str):
    return datetime.strptime(f"{fecha_str} {hora_str}", "%d/%m/%y %H:%M")

# Procesamiento inicial del archivo TXT
filas = []
with open(input_filtered_file, 'r', encoding='utf-8') as f:
    for linea in f:
        linea = linea.strip()
        if not linea or " - E-Zeus:" not in linea:
            continue
        
        try:
            cabecera, resto = linea.split(" - E-Zeus:", 1)
            fecha, hora = cabecera.strip().split()
            resto = resto.strip().replace("E-Zeus-", "", 1)
            
            ubicacion_part, resto2 = resto.split(" (", 1)
            ubicacion = ubicacion_part.strip()
            
            contenido_parentesis, detalle = resto2.split(")", 1)
            partes_parentesis = contenido_parentesis.split(" - ", 1)
            sensor = partes_parentesis[0].strip()
            resto_paren = partes_parentesis[1].strip() if len(partes_parentesis) > 1 else ""
            
            match = re.search(r'(\S+)(?=\s+GRADO)', resto_paren)
            id_val = match.group(1) if match else resto_paren.split()[-1] if resto_paren else ""
            
            filas.append({
                "Fecha": fecha,
                "Hora": hora,
                "ubicacion": ubicacion,
                "sensor": sensor,
                "id": id_val,
                "detalle": detalle
            })
            
        except Exception as e:
            continue

# Eliminar duplicados
filas_unicas = []
vistos = set()
for fila in filas:
    key = (fila["Fecha"], fila["Hora"], fila["ubicacion"], fila["sensor"], fila["id"], fila["detalle"])
    if key not in vistos:
        vistos.add(key)
        filas_unicas.append(fila)

# Procesar detalles
for fila in filas_unicas:
    detalle = fila["detalle"]
    
    sector_match = re.search(r'EL SENSOR\s+(.*?)\s+DEL EQUIPO', detalle, re.IGNORECASE)
    fila["sector"] = sector_match.group(1).strip() if sector_match else ""
    
    if re.search(r'ESTA FUERA', detalle, re.IGNORECASE):
        fila["estado"] = "Alarmado"
    elif re.search(r'HA VUELTO', detalle, re.IGNORECASE):
        fila["estado"] = "desalarmado"
    else:
        fila["estado"] = ""
    
    valor_match = re.search(r'\*(.*?)\*', detalle)
    fila["valor"] = valor_match.group(1).strip() if valor_match else ""
    
    del fila["detalle"]

# Escribir CSV intermedio
with open(output_csv_file, 'w', newline='', encoding='utf-8') as csvfile:
    writer = csv.DictWriter(csvfile, fieldnames=["Fecha", "Hora", "ubicacion", "sensor", "id", "sector", "estado", "valor"])
    writer.writeheader()
    writer.writerows(filas_unicas)

# Procesamiento para tabla_filtrada
eventos = []
for fila in filas_unicas:
    dt = convertir_datetime(fila["Fecha"], fila["Hora"])
    eventos.append({
        'dt': dt,
        'ubicacion': fila["ubicacion"],
        'sensor': fila["sensor"],
        'id': fila["id"],
        'sector': fila["sector"],
        'estado': fila["estado"],
        'valor': fila["valor"]
    })

eventos.sort(key=lambda x: x['dt'])

# Almacenar períodos y alarmas activas
periodos = []
alarmas_activas = {}

for evento in eventos:
    clave = (evento['ubicacion'], evento['sensor'], evento['id'], evento['sector'])
    
    if evento['estado'] == 'Alarmado':
        if clave not in alarmas_activas:
            alarmas_activas[clave] = {
                'inicio': evento['dt'],
                'valor_inicio': evento['valor']
            }
            
    elif evento['estado'] == 'desalarmado' and clave in alarmas_activas:
        alarma = alarmas_activas.pop(clave)
        inicio = alarma['inicio']
        fin = evento['dt']
        
        # Dividir en días
        current = inicio
        while current <= fin:
            dia_fin = min(fin, datetime(current.year, current.month, current.day) + timedelta(days=1) - timedelta(seconds=1))
            
            periodo = {
                'fecha_inicio': current.strftime("%d/%m/%y"),
                'hora_inicio': current.strftime("%H:%M"),
                'fecha_fin': dia_fin.strftime("%d/%m/%y") if dia_fin.date() == fin.date() else "N/A",
                'hora_fin': dia_fin.strftime("%H:%M") if dia_fin.date() == fin.date() else "N/A",
                'ubicacion': clave[0],
                'sensor': clave[1],
                'id': clave[2],
                'sector': clave[3],
                'valor_inicio': alarma['valor_inicio'],
                'valor_fin': evento['valor'] if dia_fin.date() == fin.date() else "N/A"
            }
            
            periodos.append(periodo)
            current = dia_fin + timedelta(seconds=1)

# Manejar alarmas no resueltas - VERSIÓN CORREGIDA
for clave, alarma in alarmas_activas.items():
    # Solo registrar el día inicial de la alarma no resuelta
    periodos.append({
        'fecha_inicio': alarma['inicio'].strftime("%d/%m/%y"),
        'hora_inicio': alarma['inicio'].strftime("%H:%M"),
        'fecha_fin': "N/A",
        'hora_fin': "N/A",
        'ubicacion': clave[0],
        'sensor': clave[1],
        'id': clave[2],
        'sector': clave[3],
        'valor_inicio': alarma['valor_inicio'],
        'valor_fin': "N/A"
    })

# Eliminar duplicados diarios y consolidar
consolidados = defaultdict(list)
for p in periodos:
    clave = (p['ubicacion'], p['sensor'], p['id'], p['sector'], p['fecha_inicio'])
    consolidados[clave].append(p)

# Escribir archivo final
with open(tabla_filtrada, 'w', newline='', encoding='utf-8') as f:
    writer = csv.DictWriter(f, fieldnames=['fecha_inicio', 'hora_inicio', 'fecha_fin', 'hora_fin',
                                          'ubicacion', 'sensor', 'id', 'sector', 'valor_inicio', 'valor_fin'])
    writer.writeheader()
    
    for grupo in consolidados.values():
        primer = min(grupo, key=lambda x: convertir_datetime(x['fecha_inicio'], x['hora_inicio']))
        ultimo = max(grupo, key=lambda x: convertir_datetime(x['fecha_fin'], x['hora_fin']) if x['fecha_fin'] != 'N/A' else datetime.min)
        
        fila = {
            'fecha_inicio': primer['fecha_inicio'],
            'hora_inicio': primer['hora_inicio'],
            'fecha_fin': ultimo['fecha_fin'] if ultimo['fecha_fin'] != 'N/A' else 'N/A',
            'hora_fin': ultimo['hora_fin'] if ultimo['hora_fin'] != 'N/A' else 'N/A',
            'ubicacion': primer['ubicacion'],
            'sensor': primer['sensor'],
            'id': primer['id'],
            'sector': primer['sector'],
            'valor_inicio': primer['valor_inicio'],
            'valor_fin': ultimo['valor_fin']
        }
        writer.writerow(fila)

print(f"Tabla filtrada generada: {tabla_filtrada}")