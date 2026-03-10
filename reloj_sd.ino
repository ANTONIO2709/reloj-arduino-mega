#include <Elegoo_GFX.h>    // Librería básica de gráficos
#include <Elegoo_TFTLCD.h> // Librería específica para el Shield TFT
#include <SD.h>            // Librería para manejar la tarjeta SD
#include <SPI.h>           // Protocolo de comunicación para la SD
#include <Wire.h>          // Protocolo I2C (Pines 20/21 en Mega)
#include <RTClib.h>        // Librería para el reloj DS1307/DS3231
#include <DHT.h>             // Librería para el sensor DHT11

// --- DEFINICIÓN DE PINES ---
#define LCD_CS A3 
#define LCD_CD A2 
#define LCD_WR A1 
#define LCD_RD A0 
#define LCD_RESET A4 

#define SD_CS 10 // Chip Select de la SD

// Inicializamos los objetos de hardware
Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET); 
RTC_DS3231 rtc; // Cambiado a DS3231 para soporte total
#define DHTPIN 22
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#define BUZZER_PIN 23

// --- VARIABLES DE TIEMPO ---
DateTime now_time;
unsigned long lastBackgroundChange = 0;
const unsigned long backgroundInterval = 30000; 
bool refreshBackground = true;
bool sd_present = false;
bool rtc_present = false;
bool rtc_is_ds3231 = false; // Nueva variable para el modelo
bool dht_present = false;
float maxTemp = -999.0;
float minTemp = 999.0;
int lastDay = -1;

// Nombres de los días de la semana en español
const char daysOfTheWeek[7][4] = {"DOM", "LUN", "MAR", "MIE", "JUE", "VIE", "SAB"};

void setup(void) {
  Serial.begin(9600);
  Serial.println(F("Reloj Arduino MEGA - SD SoftSPI + Sensores"));

  // IMPORTANTE: En el Mega, para que el SPI funcione (incluso por software), 
  // el pin 53 DEBE ser salida.
  pinMode(53, OUTPUT);

  tft.reset();
  uint16_t identifier = tft.readID();
  if (identifier == 0x0101) identifier = 0x9341; 

  tft.begin(identifier);
  tft.setRotation(1); 
  tft.fillScreen(0x0000); 
  tft.setTextSize(2);
  tft.setTextColor(0xFFFF);
  tft.setCursor(10, 10);
  tft.println("Diagnostico MEGA (SoftSPI):");

  // Iniciar SD (Ahora usa SoftSPI gracias a la modificacion de la libreria)
  Serial.print(F("Iniciando SD (Pines 10-13)..."));
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Error!"));
    tft.setTextColor(0xF800); 
    tft.println("- SD: No encontrada (Mala conexion)");
    sd_present = false;
  } else {
    Serial.println(F("OK"));
    tft.setTextColor(0x07E0); 
    tft.println("- SD: OK (Software SPI)");
    sd_present = true;
  }

  // Iniciar RTC (Pines 20/21 en Mega)
  if (!rtc.begin()) {
    tft.setTextColor(0xF800); 
    tft.println("- RTC: No encontrado");
    rtc_present = false;
  } else {
    rtc_present = true;
    Serial.print(F("Diagnostico I2C (RTC): "));
    Wire.beginTransmission(0x68);
    Wire.write(0x00);
    Wire.endTransmission();
    Wire.requestFrom(0x68, 19);
    for (int i = 0; i < 19; i++) {
      uint8_t reg = Wire.read();
      Serial.print(reg, HEX); Serial.print(" ");
      // Verificamos si hay datos en los registros de temperatura (0x11 o 0x12)
      if ((i == 0x11 || i == 0x12) && reg > 0 && reg < 0xFF) rtc_is_ds3231 = true;
    }
    Serial.println();

    if (rtc_is_ds3231) {
      tft.setTextColor(0x07E0); 
      tft.println("- RTC: OK (DS3231)");
      
      // Forzar conversion de temperatura (CONV bit)
      Wire.beginTransmission(0x68);
      Wire.write(0x0E);
      Wire.endTransmission();
      Wire.requestFrom(0x68, 1);
      uint8_t ctrl = Wire.read();
      Wire.beginTransmission(0x68);
      Wire.write(0x0E);
      Wire.write(ctrl | 0x20); // Bit 5: CONV
      Wire.endTransmission();
    } else {
      tft.setTextColor(0xFFE0); 
      tft.println("- RTC: OK (Duda DS1307?)");
    }
    
    // --- AJUSTE DE HORA ---
    // Si la hora esta mal, quita las "//" de la linea de abajo, sube el codigo, 
    // y luego vuelve a poner las "//" y sube el codigo otra vez.
    // 
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 

    if (rtc.lostPower()) {
      Serial.println(F("RTC perdio la energia, ajustando hora..."));
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  // Iniciar Zumbador y pitar al inicio
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);

  // Iniciar DHT11
  dht.begin();
  float t = dht.readTemperature();
  if (isnan(t)) {
    tft.setTextColor(0xF800); 
    tft.println("- DHT11: No encontrado / Error");
    dht_present = false;
  } else {
    tft.setTextColor(0x07E0); 
    tft.println("- DHT11: OK");
    dht_present = true;
  }

  delay(3000); 
  Serial.println(F("Sistema iniciado."));
}

void loop() {
  // Obtener tiempo real del RTC o simularlo si no hay
  if (rtc_present) {
    now_time = rtc.now(); 
  } else {
    unsigned long s = millis() / 1000;
    now_time = DateTime(2024, 1, 1, (s/3600)%24, (s/60)%60, s%60);
  }
  
  unsigned long currentMillis = millis();

  // Control del fondo de pantalla y reloj
  if (currentMillis - lastBackgroundChange >= backgroundInterval || refreshBackground) {
    Serial.println(F("--- Cambio de Fondo ---"));
    lastBackgroundChange = currentMillis;
    refreshBackground = false;
    
    if (sd_present) {
      tft.setRotation(2); 
      cargarSiguienteFondo(); 
    } else {
      tft.fillScreen(0x001F); 
    }
    
    tft.setRotation(1); 
    dibujarReloj(true); 
  } else {
    dibujarReloj(false); 
  }
  
  // --- PÍTIDO HORARIO ---
  static int lastHour = -1;
  if (now_time.hour() != lastHour) {
    if (lastHour != -1) { // No pitar al arrancar
      Serial.println(F("¡Hora en punto! Pitido..."));
      digitalWrite(BUZZER_PIN, HIGH);
      delay(500);
      digitalWrite(BUZZER_PIN, LOW);
    }
    lastHour = now_time.hour();
  }

  // --- TEMPERATURAS MÁXIMA / MÍNIMA ---
  if (dht_present) {
    float currentTemp = dht.readTemperature();
    if (!isnan(currentTemp)) {
      // Reinicio diario a las 00:00
      if (now_time.day() != lastDay) {
        maxTemp = currentTemp;
        minTemp = currentTemp;
        lastDay = now_time.day();
        Serial.println(F("Nuevo día: Reiniciando Max/Min"));
      }
      
      if (currentTemp > maxTemp) maxTemp = currentTemp;
      if (currentTemp < minTemp) minTemp = currentTemp;
    }
  }

  delay(500); 
}

void cargarSiguienteFondo() {
  static File root;
  if (!root) root = SD.open("/");
  
  static char lastFile[13] = "";
  int attempts = 0;
  bool found = false;
  
  while (attempts < 50) { 
    attempts++;
    File entry = root.openNextFile();
    if (!entry) { 
      Serial.println(F("--- Fin de archivos, reiniciando busqueda ---"));
      root.rewindDirectory(); 
      continue; 
    }
    
    char* name = entry.name();
    
    // Ignorar si es directorio o archivo oculto (empieza por .)
    if (entry.isDirectory() || name[0] == '.') {
      entry.close();
      continue;
    }

    Serial.print(F("Encontrado: ")); Serial.println(name);

    int len = strlen(name);
    if (len > 4 && strcasecmp(name + len - 4, ".BMP") == 0 && strcmp(name, lastFile) != 0) {
      Serial.print(F("Cargando: ")); Serial.println(name);
      strcpy(lastFile, name);
      bmpDraw(name, 0, 0);
      found = true;
      entry.close();
      break;
    }
    entry.close();
  }
  
  if (!found) {
    Serial.println(F("No se encontraron fotos .BMP validas."));
    tft.fillScreen(0x001F); // Fondo azul si no hay fotos
  }
}

void dibujarReloj(bool fullRedraw) {
  static int lastM = -1;

  if (now_time.minute() != lastM || fullRedraw) {
    lastM = now_time.minute();

    int boxW = 240;
    int boxH = 130; // Aumentamos altura para añadir la altitud
    int boxX = (tft.width() - boxW) / 2;
    int boxY = (tft.height() - boxH) / 2;

    // Caja de fondo
    tft.fillRect(boxX, boxY, boxW, boxH, 0x0000); 
    tft.drawRect(boxX, boxY, boxW, boxH, 0xFFFF);

    // FECHA Y DÍA (Arriba)
    tft.setTextSize(2);
    tft.setTextColor(0xFFE0); // Amarillo
    tft.setCursor(boxX + 25, boxY + 10);
    tft.print(daysOfTheWeek[now_time.dayOfTheWeek()]);
    tft.print(" ");
    if (now_time.day() < 10) tft.print("0");
    tft.print(now_time.day());
    tft.print("/");
    if (now_time.month() < 10) tft.print("0");
    tft.print(now_time.month());
    tft.print("/");
    tft.print(now_time.year());

    // HORA (Grande al centro)
    tft.setCursor(boxX + 45, boxY + 35);
    tft.setTextColor(0xFFFF);
    tft.setTextSize(5);
    if (now_time.hour() < 10) tft.print("0");
    tft.print(now_time.hour());
    tft.print(":");
    if (now_time.minute() < 10) tft.print("0");
    tft.print(now_time.minute());

    // DATOS SENSORES (Abajo)
    tft.setTextSize(2);
    
    // Lectura DHT11
    float tempDHT = dht.readTemperature();
    float humDHT = dht.readHumidity();
    
    tft.setTextColor(0x07E0); // Verde
    tft.setCursor(boxX + 10, boxY + 85);
    tft.print("TEMP: "); 
    if (isnan(tempDHT)) tft.print("--"); else tft.print(tempDHT, 1);
    tft.print(" C");

    tft.setTextColor(0x07FF); // Cyan
    tft.setCursor(boxX + 10, boxY + 105);
    tft.print("HUM:  "); 
    if (isnan(humDHT)) tft.print("--"); else tft.print(humDHT, 0);
    tft.print(" %");

    // Max/Min (Pequeño al lado)
    tft.setTextSize(1);
    tft.setTextColor(0xF800); // Rojo para Max
    tft.setCursor(boxX + 165, boxY + 85);
    tft.print("MAX: "); tft.print(maxTemp, 1);
    
    tft.setTextColor(0x001F); // Azul para Min
    tft.setCursor(boxX + 165, boxY + 105);
    tft.print("MIN: "); tft.print(minTemp, 1);
  }
}

// --- FUNCIÓN BMP DRAW ---
#define BUFFPIXEL 20
void bmpDraw(char *filename, int x, int y) {
  File     bmpFile;
  int      bmpWidth, bmpHeight;
  uint8_t  bmpDepth;
  uint32_t bmpImageoffset;
  uint32_t rowSize;
  uint8_t  sdbuffer[3*BUFFPIXEL];
  uint16_t lcdbuffer[BUFFPIXEL];
  uint8_t  buffidx = sizeof(sdbuffer);
  boolean  goodBmp = false;
  boolean  flip    = true;
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0;
  uint8_t  lcdidx = 0;
  boolean  first = true;

  if((bmpFile = SD.open(filename)) == NULL) return;

  if(read16(bmpFile) == 0x4D42) {
    read32(bmpFile);
    (void)read32(bmpFile);
    bmpImageoffset = read32(bmpFile);
    read32(bmpFile);
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);

    if(read16(bmpFile) == 1) {
      bmpDepth = read16(bmpFile);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) {
        goodBmp = true;
        rowSize = (bmpWidth * 3 + 3) & ~3;
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }
        w = bmpWidth; h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;
        tft.setAddrWindow(x, y, x+w-1, y+h-1);
        for (row=0; row<h; row++) {
          if(flip) pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) {
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer);
          }
          for (col=0; col<w; col++) {
            if (buffidx >= sizeof(sdbuffer)) {
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0;
            }
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            lcdbuffer[lcdidx++] = tft.color565(r,g,b);
            if (lcdidx >= BUFFPIXEL) {
              tft.pushColors(lcdbuffer, BUFFPIXEL, first);
              lcdidx = 0; first = false;
            }
          }
        }
        if (lcdidx > 0) tft.pushColors(lcdbuffer, lcdidx, first);
      }
    }
  }
  bmpFile.close();
}

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();
  return result;
}
