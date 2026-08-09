## Реализация видеосистемы на FPGA с обработкой кадров (OV7670 + SystemVerilog + HLS)

### Проект представляет собой систему захвата видео с камеры OV7670, потоковую обработку данных (фильтрация изображения) и вывод изображения в HDMI.

### Технический стек
* **Языки:** SystemVerilog, C++
* **САПР:** Vivado, Vitis, Vitis HLS
* **Аппаратная платформа:** ZYNQ MINI XC7Z020CLG400-2

### Архитектура системы
<img width="1854" height="667" alt="BlockDesign" src="https://github.com/user-attachments/assets/46265977-db15-4f2c-8f72-15acc2e0f8c4" />

### Архитектура программно-аппаратного комплекса

### Аппаратная часть (FPGA / Programmable Logic)
#### rtl_modules (SystemVerilog):
  1. **ov7670_camera:** Захват видеопотока с камеры ov7670 в формате YUV. Преобразование в формат RGB888 и передача видеопотока по AXI-STREAM на следующий модуль обработки.
  2. **GaussFilterSystemVerilog:** Модуль фильтра Гаусса с окном 3х3.
  3. **MyScreen:** Модуль, который выдаёт видеопоток типа "шахматная доска" по AXI-STREAM.     
#### hls_ip_blocks (C++ / Vitis HLS):
  1. **MedianFilter:** Модуль медианного фильтра с окном 3х3, с возможность динамической настройки порога через AXILite.
      
### Программная часть (ARM Cortex-A9 / Processing System)
#### drivers (C++ / Vitis):
  1. **ov7670_camera:** Драйвер конфигурации камеры ov7670 по I2C.
  2. **MyScreen:** Драйвер конфигурации модуля MyScreen.
  3. **MedianFilter_hls:** Драйвер конфигурации модуля MedianFilter и методы управления данным модулем.
#### platform:
  1. **GenericInterruptController:** Инициализация подсистемы аппаратных прерываний.
  2. **I2C:** Инициализация интерфейса I2C в Zynq.
  3. **UART:** Инициализация интерфейса UART в Zynq.

 
