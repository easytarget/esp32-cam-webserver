#include "storage.h"

void CLStorage::listDir(const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fsStorage->open(dirname);
  if(!root){
    Serial.println("- failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println(" - not a directory");
    return;
  }
  
  root.rewindDirectory();

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}


bool CLStorage::init() {
#ifdef USE_LittleFS
  return fsStorage->begin(FORMAT_LITTLEFS_IF_FAILED);
#else
  if(!fsStorage->begin("/root", true, false, SDMMC_FREQ_DEFAULT)) return false;

  uint8_t cardType = fsStorage->cardType();

  switch(cardType) {
    case CARD_NONE:
      Serial.println("No SD card attached");
      return false;
    case CARD_MMC:
      Serial.print("MMC");
      break;
    case CARD_SD:
      Serial.print("SDSC");
      break;
    case CARD_SDHC:
      Serial.print("SDHC");
      break;
    default:
      Serial.println("Unknown Type");
      break;    
  }

  Serial.printf(" card size: %lluMB\n", getSize());
  
  return true;
#endif
}


int CLStorage::readFileToString(char *path, String *s)
{
	File file = fsStorage->open(path);
	if (!file)
		return FAIL;

	while (file.available())
	{
		char charRead = file.read();
		*s += charRead;
	}
  file.close();
	return OK;
}

int CLStorage::getSize() {
  return (int) (fsStorage->totalBytes() / pow(1024, STORAGE_UNITS));
}

int CLStorage::getUsed() {
  return (int) (fsStorage->usedBytes() / pow(1024, STORAGE_UNITS));
}

int CLStorage::capacityUnits() {
  return STORAGE_UNITS;
}

CLStorage Storage;
