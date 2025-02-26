import re
import csv

# Parámetros: deben coincidir con los usados en el filtrado previo
mes = 1    # mes deseado (1-12)
año = 25   # año deseado en dos dígitos (ej. 25 para 2025)

# Archivo TXT de entrada (temporal) y de salida CSV
input_filtered_file = f"filtrado_mes_{mes}_año_{año}.txt"
output_csv_file = f"csv_filtrado_mes_{mes}_año_{año}.csv"

# Lista para almacenar las filas extraídas
filas = []

with open(input_filtered_file, 'r', encoding='utf-8') as f:
    for linea in f:
        linea = linea.strip()
        if not linea:
            continue
        
        # Se espera que la línea contenga " - E-Zeus:"
        if " - E-Zeus:" not in linea:
            continue
        
        try:
            # Separa la cabecera (fecha y hora) del resto
            cabecera, resto = linea.split(" - E-Zeus:", 1)
            cabecera = cabecera.strip()  # Ejemplo: "2/1/25 03:39"
            fecha, hora = cabecera.split()  # Fecha: "2/1/25", Hora: "03:39"
            
            resto = resto.strip()
            # Remover el prefijo "E-Zeus-" si existe
            if resto.startswith("E-Zeus-"):
                resto = resto[len("E-Zeus-"):]
            
            # Ubicación: desde el inicio hasta el primer " ("
            ubicacion_part, resto2 = resto.split(" (", 1)
            ubicacion = ubicacion_part.strip()
            
            # Extraer contenido dentro de paréntesis y el detalle (lo que sigue)
            contenido_parentesis, detalle = resto2.split(")", 1)
            detalle = detalle.strip()
            
            # Dentro de los paréntesis se espera un formato "sensor - ...".
            partes_parentesis = contenido_parentesis.split(" - ", 1)
            if len(partes_parentesis) < 2:
                continue
            sensor = partes_parentesis[0].strip()
            resto_paren = partes_parentesis[1].strip()
            
            # Extraer la id:
            # Primero, se intenta obtener el token justo antes de "GRADO"
            match = re.search(r'(\S+)(?=\s+GRADO)', resto_paren)
            if match:
                id_val = match.group(1)
            else:
                # Si "GRADO" no se encuentra, se toma el último token del contenido de los paréntesis
                tokens = resto_paren.split()
                id_val = tokens[-1] if tokens else ""
            
            filas.append({
                "Fecha": fecha,
                "Hora": hora,
                "ubicacion": ubicacion,
                "sensor": sensor,
                "id": id_val,
                "detalle": detalle
            })
            
        except Exception as e:
            # Si ocurre algún error al parsear la línea, se omite
            continue

# Eliminación de duplicados: se conserva únicamente una fila por cada conjunto único de valores
filas_unicas = []
vistos = set()
for fila in filas:
    key = (fila["Fecha"], fila["Hora"], fila["ubicacion"], fila["sensor"], fila["id"], fila["detalle"])
    if key not in vistos:
        vistos.add(key)
        filas_unicas.append(fila)

# Particionar la columna "detalle" en sector, estado y valor
for fila in filas_unicas:
    detalle = fila["detalle"]
    
    # Extraer sector: lo que está entre "EL SENSOR" y "DEL EQUIPO"
    sector = ""
    m_sector = re.search(r'EL SENSOR\s+(.*?)\s+DEL EQUIPO', detalle, re.IGNORECASE)
    if m_sector:
        sector = m_sector.group(1).strip()
    
    # Extraer estado: si contiene "ESTA FUERA" se traduce a "Alarmado",
    # si contiene "HA VUELTO" se traduce a "desalarmado"
    estado = ""
    if re.search(r'ESTA FUERA', detalle, re.IGNORECASE):
        estado = "Alarmado"
    elif re.search(r'HA VUELTO', detalle, re.IGNORECASE):
        estado = "desalarmado"
    
    # Extraer valor: lo que está entre asteriscos "*"
    valor = ""
    m_valor = re.search(r'\*(.*?)\*', detalle)
    if m_valor:
        valor = m_valor.group(1).strip()
    
    # Agregar las nuevas columnas a la fila
    fila["sector"] = sector
    fila["estado"] = estado
    fila["valor"] = valor
    
    # Se descarta el detalle original
    del fila["detalle"]

# Escribir la información extraída en un archivo CSV con las columnas actualizadas
with open(output_csv_file, 'w', newline='', encoding='utf-8') as csvfile:
    fieldnames = ["Fecha", "Hora", "ubicacion", "sensor", "id", "sector", "estado", "valor"]
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer.writeheader()
    for fila in filas_unicas:
        writer.writerow(fila)

print(f"Archivo CSV generado: {output_csv_file}")

