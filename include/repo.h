#ifndef REPO_H
#define REPO_H

#include "minigit.h"

// Инициализирует новый репозиторий на диске (низкоуровневая функция)
MinigitStatus init_repo_disk(void);

// Проверяет, существует ли репозиторий в текущей директории
int repo_exists(void);

#endif