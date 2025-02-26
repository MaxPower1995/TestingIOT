import pandas as pd

def filtrar_csv(input_file, output_file):
    # Leer el archivo CSV
    df = pd.read_csv(input_file, delimiter=';')
    
    # Filtrar filas donde la primera columna contiene "GRADO 2" o "Detector"
    filtro = df.iloc[:, 0].str.contains("GRADO 2|Detector", case=False, na=False)
    df_filtrado = df[filtro]
    
    # Descartar filas donde la columna 9 contiene "Mantenimiento"
    df_filtrado = df_filtrado[df_filtrado.iloc[:, 8] != "Mantenimiento"]
    
    # Guardar el nuevo archivo filtrado
    df_filtrado.to_csv(output_file, index=False, sep=';')
    
    print(f"Archivo filtrado guardado en: {output_file}")

# Ejemplo de uso con la ruta especificada
filtrar_csv(r"C:\Users\emset\Downloads\enero.csv", r"C:\Users\emset\Downloads\enero_filtrado.csv")

