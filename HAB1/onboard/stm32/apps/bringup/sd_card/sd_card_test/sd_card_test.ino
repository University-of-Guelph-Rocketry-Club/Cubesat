// Standalone NUCLEO-F411RE hardware check; not the flight logger.
#include <SPI.h>
#include <SD.h>
#include <string.h>

static const uint32_t SD_CS = PB6; // Arduino D10
static const char MESSAGE[] = "URC-43: SD write/read test passed.\r\n";
static bool passed = false;

static void fail(const char *reason)
{
    Serial.print("FAIL: ");
    Serial.println(reason);
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    Serial.begin(115200);
    delay(2000);
    Serial.println("URC-43 NUCLEO-F411RE SD bench test");

    // Explicit wiring: D11=PA7, D12=PA6, D13=PA5.
    SPI.setMOSI(PA7);
    SPI.setMISO(PA6);
    SPI.setSCLK(PA5);
    if (!SD.begin(1000000U, SD_CS)) {
        Sd2Card diagnosticCard;
        SdVolume diagnosticVolume;
        if (!diagnosticCard.init(SPI_QUARTER_SPEED, SD_CS)) {
            Serial.print("Card error code=0x");
            Serial.print(diagnosticCard.errorCode(), HEX);
            Serial.print(" response=0x");
            Serial.println(diagnosticCard.errorData(), HEX);
            fail("SPI card initialization; check pin positions, power and seating");
        } else if (!diagnosticVolume.init(diagnosticCard)) {
            fail("card responds, but FAT16/FAT32 volume mount failed");
        } else {
            fail("card and volume respond; filesystem open or speed issue");
        }
        return;
    }

    char filename[13];
    bool found = false;
    for (unsigned int index = 0; index < 10000; ++index) {
        snprintf(filename, sizeof(filename), "TEST%04u.TXT", index);
        if (!SD.exists(filename)) {
            found = true;
            break;
        }
    }
    if (!found) {
        fail("test filename space exhausted; no files overwritten");
        return;
    }

    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        fail("create file");
        return;
    }
    const size_t expected = sizeof(MESSAGE) - 1;
    const size_t written = file.write((const uint8_t *)MESSAGE, expected);
    file.flush();
    file.close();
    if (written != expected) {
        fail("short write");
        return;
    }

    file = SD.open(filename, FILE_READ);
    if (!file) {
        fail("reopen file");
        return;
    }
    char actual[sizeof(MESSAGE)] = {0};
    const int received = file.read((uint8_t *)actual, expected);
    const bool extra = file.available() != 0;
    file.close();
    if (received != (int)expected || extra || memcmp(actual, MESSAGE, expected)) {
        fail("read-back mismatch");
        return;
    }

    Serial.print("PASS: wrote, closed, reopened and verified ");
    Serial.println(filename);
    Serial.print(actual);
    Serial.println("Bench check only; power-cut durability is not tested.");
    passed = true;
}

void loop()
{
    // PA5 also carries SPI SCK. Only use the LED after all SD operations finish.
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(passed ? 1000 : 100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(passed ? 1000 : 100);
}
