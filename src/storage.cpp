#include "storage.h"

/// @brief List the content of a folder
/// @param dirname 
/// @param levels 
void listDir(const char * dirname, uint8_t levels){
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




/// @brief file storage initialization
/// @return true if success, or false otherwise.
bool init_storage() {
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

  Serial.printf(" card size: %lluMB\n", storageSize());

  return true;
#endif
}

/// @brief Load a file to a String
/// @param path file name
/// @param s pointer to the String buffer
/// @return OK(0) or FAIL(1)
int readFileToString(char *path, String *s)
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

int storageSize() {
  return (int) (fsStorage->totalBytes() / pow(1024, STORAGE_UNITS));
}

int storageUsed() {
  return (int) (fsStorage->usedBytes() / pow(1024, STORAGE_UNITS));
}

int capacityUnits() {
  return STORAGE_UNITS;
}
