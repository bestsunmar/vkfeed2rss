#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include "zagruzka.h"
#include "zapros.h"
#include "feed2rss.h"
#include "info.h"

void pomosch(const char *zapusk) // помощь по программе
{
	fprintf(stdout, "Использование:\n"
		"  -h - эта помощь\n"
		"  -v - версия программы\n"
		"  -i - id сообщества\n"
		"  -s - id пользователя\n"
		"  -d - домен страницы, напр. \"apiclub\"\n"
		"  -k - количество записей, максимум 100, 20 по умолчанию\n"
		"  -f - фильтр (в этой версии не работает)\n"
		"  -V - режим отладки (выводит сырой ответ API в ленте в комментариях)\n"
		"\n%s -d apiclub\n", zapusk);
}

void version() // выводит версию программы
{
	fprintf(stdout, "%s v%s - переводчик ленты сообществ ВКонтакте в RSS\nAPI v%s\n", nazvanie, VERSION, APIVERSION);
}

int main(int argc, char **argv)
{
	struct Parametry stranica; // читайте файл zapros.h
	stranica.domain = NULL; // необходимая очистка
	stranica.zagolovok = NULL;
	stranica.opisanie = NULL;
	stranica.lenta = NULL;
	stranica.info = NULL;
	stranica.id = 0;
	stranica.filter = 0; // временно нерабочая функция
	stranica.kolichestvo = 20; // значение count по умолчанию, см. wall.get в документации к API VK
	stranica.verbose = false;

	if (argc == 1) { // если нет аргументов, то вывести помощь
		pomosch(argv[0]);
		return 0;
	}
	else { // если аргументы есть, то будут обрабатываться
		int c;
		while ((c = getopt(argc, argv, "hvi:s:d:k:fV")) != -1) { // getopt как и обычно
			switch (c) {
				case 'h': // помощь
					pomosch(argv[0]);
					return 0;
				case 'v': // вывод версии
					version();
					return 0;
				case 'i': // id группы
					stranica.id = atoll(optarg);
					stranica.type = true;
					break;
				case 's': // id пользователя
					stranica.id = atoll(optarg);
					stranica.type = false;
					break;
				case 'd': // домен страницы
					if (stranica.id == 0) // если id введён не был, то проверка домена
						stranica.domain = optarg;
					break;
				case 'k': // количество постов в ленте
					if (atoi(optarg) > 100 || atoi(optarg) < 0) {
						fprintf(stderr, "%s: количество записей на страницу должно быть не больше 100 и не меньше 0, выбрано значение по умолчанию\n", nazvanie);
						stranica.kolichestvo = 20; // костыль
					}
					else stranica.kolichestvo = atoi(optarg);
					break;
				case 'V': // verbose
					stranica.verbose = true;
					break;
			}
		}
	}
	
	// сейчас мы получим JSON вывод стены и потом будем его парсить
	
	char *url_zaprosa_lenty = poluchit_url_zaprosa_lenty(stranica); // создать ссылку запроса для API чтобы получить ленту
	if (url_zaprosa_lenty == NULL) {
		fprintf(stderr, "%s: произошла непредвиденная ошибка при образовании запроса\n", nazvanie);
		return -1;
	}
	char *url_zaprosa_info_stranicy = poluchit_url_zaprosa_info_stranicy(stranica); // создать ссылку запроса для API чтобы получить информацию о странице
	if (url_zaprosa_info_stranicy == NULL) {
		fprintf(stderr, "%s: произошла непредвиденная ошибка при образовании запроса\n", nazvanie);
		return -1;
	}

	stranica.lenta = zagruzka_lenty(url_zaprosa_lenty); // загружаем ленту
	if (stranica.lenta == NULL) {
		fprintf(stderr, "%s: произошла непредвиденная ошибка при загрузке ленты\n", nazvanie);
		return -1;
	}
	
	stranica.info = zagruzka_lenty(url_zaprosa_info_stranicy); // загружаем информацию о странице
	if (stranica.info == NULL) {
		fprintf(stderr, "%s: произошла непредвиденная ошибка при загрузке информации о страницы\n", nazvanie);
		return -1;
	}

	stranica.zagolovok = poluchit_zagolovok(stranica); // полученное stranica.info надо обработать и записать данные
	if (stranica.zagolovok == NULL) {
		fprintf(stderr, "%s: произошла непредвиденная ошибка при обработке данных\n", nazvanie);
		return -1;
	}
	
	stranica.opisanie  = poluchit_opisanie(stranica);
	if (stranica.opisanie == NULL) {
		fprintf(stderr, "%s: произошла непредвиденная ошибка при обработке данных\n", nazvanie);
		return -1;
	}

	if (osnova_rss(stranica) == -1) { // сделать основу для RSS ленты
		fprintf(stderr, "%s: произошла непредвиденная ошибка при записи RSS ленты\n", nazvanie);
		return -1;
	}

	if (obrabotka(stranica) == -1) { // обработать ленту для RSS, заполняет RSS ленту записями
		fprintf(stderr, "%s: произошла непредвиденная ошибка при записи записей в RSS ленты\n", nazvanie);
		return -1;
	}

	return 0;
}
