## Mejora progresiva del sistema Biodata

1. **Registrar datos crudos y calibraciones básicas**  
   - Guardar cada bloque de muestras antes de mapearlo a notas.  
   - Documentar la configuración (threshold, escala, canal) y registrar eventos de usuario.  
   - Permite analizar fuera de línea y repetir pruebas.

2. **Procesamiento digital más riguroso**  
   - Aplicar filtrado paso bajo digital, ventanas móviles y métricas estadísticas estándar (RMS, PSD).  
   - Incluir análisis espectral o por bandas para caracterizar la señal eléctrica.  
   - Mejora la detección de eventos sin modificar el hardware.

3. **Optimizar electrodos y etapa analógica**  
   - Usar electrodos diferenciales y un amplificador de instrumentación con filtrado analógico.  
   - Reducir artefactos por ruido común y estabilizar la señal antes del ADC.  
   - Requiere rediseñar la interfaz planta‑sensor pero mantiene el resto del sistema.

4. **Adquirir con un ADC dedicado de alta resolución**  
   - Integrar un convertidor como ADS1115/ADS131M o ADS1299 para muestreo sincrónico.  
   - Configurar tasas de muestreo controladas y resolución de 16‑24 bits.  
   - Implica rediseño de hardware, firmware y sincronización con el resto del flujo.

5. **Validación cruzada con instrumentación científica**  
   - Medir en paralelo con un registrador bioeléctrico/comercial certificado.  
   - Comparar amplitud, espectro y respuesta a estímulos para validar rigurosamente el sistema.  
  - Involucra equipamiento externo, protocolos de prueba y análisis comparativo avanzado.


