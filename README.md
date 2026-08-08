## Реализация видеосистемы на FPGA с обработкой кадров (OV7670 + SystemVerilog + HLS)

### Проект представляет собой систему захвата видео с камеры OV7670, потоковую обработку данных (фильтрация изображения) и вывод изображения в HDMI.

### Технический стек
* **Языки:** SystemVerilog, C++
* **САПР:** Vivado, Vitis, Vitis HLS
* **Аппаратная платформа:** ZYNQ MINI XC7Z020CLG400-2

### Архитектура системы
<img width="1854" height="667" alt="BlockDesign" src="https://github.com/user-attachments/assets/46265977-db15-4f2c-8f72-15acc2e0f8c4" />

### Описание реализованных IP модулей
1. **camera_capture (SystemVerilog):** Захват видеопотока с камеры ov7670 в формате YUV. Преобразование в формат RGB888 и передача видеопотока по AXI-STREAM на следующий модуль обработки.
2. **I2C (C++):** Настройка модуля, находящегося в ARM XC7Z020CLG400-2, для инициализации камеры ov7670.
3. **GenericInterruptController (С++):** Настройка контроллера прерываний GIC.
4. **GausseFilterSystemVerilog (SystemVerilog):** Модуль фильтра Гаусса с окном 3х3.
5. **MyScreen (SystemVerilog):** Модуль, который выдаёт видеопоток типа "шахматная доска" по AXI-STREAM.    
