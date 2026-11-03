// Read-only recovery aid. Never formats, creates, deletes, or repairs files.
#include <SPI.h>
#include <SD.h>
void setup()
{
    Serial.begin(115200);
    delay(2000);
    SPI.setMOSI(PA7);
    SPI.setMISO(PA6);
    SPI.setSCLK(PA5);
    Serial.println("URC-43 read-only SD recovery inspection");
    if (!SD.begin(1000000U, PB6)) {
        Serial.println("FAIL initialization");
        return;
    }
    File root = SD.open("/", FILE_READ);
    if (!root) {
        Serial.println("FAIL root directory");
        return;
    }
    for (;;) {
        File file = root.openNextFile();
        if (!file) {
            break;
        }
        Serial.print("FILE ");
        Serial.print(file.name());
        Serial.print(" bytes=");
        Serial.println(file.size());
        if (!file.isDirectory() && strncmp(file.name(), "LOG", 3) == 0) {
            Serial.print("RECOVERY_BEGIN ");
            Serial.println(file.name());
            while (file.available()) {
                Serial.write(file.read());
            }
            Serial.println("RECOVERY_END");
        }
        file.close();
    }
    root.close();
    Serial.println("READ_ONLY_INSPECTION_DONE");
}
void loop()
{
    delay(1000);
}
