#include <Arduino.h>
#include <user_interface.h>
#include "crash_report.h"

static const uint32_t CRASH_MAGIC = 0xC5A5C5A5;
// Bloque de memoria RTC de usuario en el que guardamos el crash (offset en
// palabras de 4 bytes, 0-127 disponibles). No se usa en ningún otro sitio del
// proyecto (ni deep sleep ni WiFiManager la tocan aquí), así que no hay
// colisión.
static const uint32_t RTC_OFFSET_WORDS = 32;

struct CrashRecord {
  uint32_t magic;
  uint32_t reason;
  uint32_t exccause;
  uint32_t epc1;
  uint32_t epc2;
  uint32_t epc3;
  uint32_t excvaddr;
  uint32_t depc;
};

extern "C" void custom_crash_callback(struct rst_info *rstInfo, uint32_t /*stackStart*/, uint32_t /*stackEnd*/) {
  CrashRecord rec;
  rec.magic = CRASH_MAGIC;
  rec.reason = rstInfo->reason;
  rec.exccause = rstInfo->exccause;
  rec.epc1 = rstInfo->epc1;
  rec.epc2 = rstInfo->epc2;
  rec.epc3 = rstInfo->epc3;
  rec.excvaddr = rstInfo->excvaddr;
  rec.depc = rstInfo->depc;
  ESP.rtcUserMemoryWrite(RTC_OFFSET_WORDS, reinterpret_cast<uint32_t *>(&rec), sizeof(rec));
}

void reportSavedCrash() {
  CrashRecord rec;
  ESP.rtcUserMemoryRead(RTC_OFFSET_WORDS, reinterpret_cast<uint32_t *>(&rec), sizeof(rec));
  if (rec.magic != CRASH_MAGIC) return;

  Serial.println(F("=== CRASH GUARDADO (reinicio anterior) ==="));
  Serial.printf("reason=%u exccause=%u\n", rec.reason, rec.exccause);
  Serial.printf("epc1=0x%08x epc2=0x%08x epc3=0x%08x\n", rec.epc1, rec.epc2, rec.epc3);
  Serial.printf("excvaddr=0x%08x depc=0x%08x\n", rec.excvaddr, rec.depc);
  Serial.println(F("==========================================="));

  rec.magic = 0;
  ESP.rtcUserMemoryWrite(RTC_OFFSET_WORDS, reinterpret_cast<uint32_t *>(&rec), sizeof(rec));
}
