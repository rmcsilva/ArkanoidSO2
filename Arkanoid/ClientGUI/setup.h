#pragma once

int setupMovementKeys(HANDLE* hRegistryKey, TCHAR* rightMovementKey, TCHAR* leftMovementKey);
int saveMovementKeys(HANDLE* hRegistryKey, TCHAR* rightMovementKey, TCHAR* leftMovementKey);

//Except filter
int filter(unsigned int code);