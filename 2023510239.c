

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strcasecmp — case-insensitive comparison */
#include <cjson/cJSON.h>

#define MAX_COUNTRIES 100
#define MAX_CITIES    1000
#define MAX_PRODUCTS  10000
#define RSS_MEM_SIZE  3   /* Replacement Selection Sort buffer size */

/*  Struct Definitions */

typedef struct {
    char   product_id[30];
    char   product_name[100];
    char   brand[50];
    char   category[50];
    char   country[50];
    char   city[50];
    double price;
    char   currency[10];
    int    stock;
    char   warehouse[30];
    char   isbn[30];
    char   description[200];
    char   extra[100];
} ProductRecord;

typedef struct {
    char country[50];
    int  city_offset;
} CountryIndex;

typedef struct {
    char country[50];
    char city[50];
    int  next_city;
    int  product_offset;
} CityIndex;

typedef struct {
    char country[50];
    char city[50];
    char product_name[100];
    int  next_product;
    long dat_offset;
} ProductIndex;

/*  Global Index Arrays  */

CountryIndex countryIndex[MAX_COUNTRIES];
CountryIndex originalCountryInput[MAX_COUNTRIES];
CityIndex    cityIndex[MAX_CITIES];
ProductIndex productIndex[MAX_PRODUCTS];

int countryCountGlobal = 0;
int cityCountGlobal    = 0;
int productCountGlobal = 0;

/*  Forward Declarations  */

void safeCopy(char *dest, cJSON *item, size_t size);
void readLine(char *buffer, int size);
char *readFile(const char *filename);

void rssCountries(int *indexes, int count);
void rssCities(int *indexes, int count);
void rssProducts(int *indexes, int count);

void createAllFilesAndIndexes(cJSON *root);
void saveUpdatedIndexes(void);

void displayCountryIndex(void);
void displayCityIndex(void);
void displayProductIndex(void);
void displaySortedIndexLevels(void);

int  findCountry(char countryName[]);
int  findCityByCountryAndCity(char countryName[], char cityName[]);

void searchProduct(char countryName[], char cityName[], char productName[]);
void searchByCountry(void);
void searchByCity(void);
void searchByProductName(void);
void insertProduct(void);
void applyReplacementSelectionSortDemo(void);

/*  Helper Utilities  */

void safeCopy(char *dest, cJSON *item, size_t size) {
    if (item != NULL && cJSON_IsString(item) && item->valuestring != NULL) {
        strncpy(dest, item->valuestring, size - 1);
        dest[size - 1] = '\0';
    } else {
        dest[0] = '\0';
    }
}

void readLine(char *buffer, int size) {
    fgets(buffer, size, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
}

char *readFile(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("File could not be opened: %s\n", filename);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    char *buffer = malloc(size + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}



static int cmpCountryFn(int a, int b) {
    return strcasecmp(countryIndex[a].country, countryIndex[b].country);
}
static int cmpCityFn(int a, int b) {
    return strcasecmp(cityIndex[a].city, cityIndex[b].city);
}
static int cmpProductFn(int a, int b) {
    return strcasecmp(productIndex[a].product_name, productIndex[b].product_name);
}

static void rssGeneric(int *indexes, int count, int (*cmp)(int, int)) {
    if (count <= 0) return;

    int *sorted  = malloc(count * sizeof(int));
    int *runEnds = malloc((count + 1) * sizeof(int));
    if (!sorted || !runEnds) {
        printf("malloc failed (rssGeneric)\n");
        free(sorted); free(runEnds); return;
    }

    int mem[RSS_MEM_SIZE];
    int frozen[RSS_MEM_SIZE];
    int memCount = 0, inputPos = 0, outputPos = 0, numRuns = 0;

    /* ── Phase 1: produce sorted runs ── */
    while (inputPos < count || memCount > 0) {

        while (memCount < RSS_MEM_SIZE && inputPos < count) {
            mem[memCount]    = indexes[inputPos++];
            frozen[memCount] = 0;
            memCount++;
        }

        int minIdx = -1;
        for (int i = 0; i < memCount; i++) {
            if (!frozen[i] && (minIdx == -1 || cmp(mem[i], mem[minIdx]) < 0))
                minIdx = i;
        }

        if (minIdx == -1) {
            /* All frozen — close run, thaw for next run */
            runEnds[numRuns++] = outputPos;
            for (int i = 0; i < memCount; i++) frozen[i] = 0;
            continue;
        }

        int winner = mem[minIdx];
        sorted[outputPos++] = winner;

        if (inputPos < count) {
            int next = indexes[inputPos++];
            frozen[minIdx] = (cmp(next, winner) < 0) ? 1 : 0;
            mem[minIdx] = next;
        } else {
            for (int j = minIdx; j < memCount - 1; j++) {
                mem[j]    = mem[j + 1];
                frozen[j] = frozen[j + 1];
            }
            memCount--;
        }
    }
    runEnds[numRuns++] = outputPos;  /* close the final run */

    /* ── Phase 2: pairwise merge until one run remains ── */
    int *temp    = malloc(count * sizeof(int));
    int *curEnds = malloc(numRuns * sizeof(int));

    if (temp && curEnds) {
        memcpy(curEnds, runEnds, numRuns * sizeof(int));
        int curRuns = numRuns;

        while (curRuns > 1) {
            int  newRuns = 0;
            int *newEnds = malloc(((curRuns + 1) / 2 + 1) * sizeof(int));
            if (!newEnds) { free(curEnds); curEnds = NULL; break; }

            int runStart = 0;
            for (int r = 0; r < curRuns; r += 2) {
                if (r + 1 >= curRuns) {
                    /* Odd run — no pair, keep as-is */
                    newEnds[newRuns++] = curEnds[r];
                    runStart = curEnds[r];
                } else {
                    int left  = runStart;
                    int mid   = curEnds[r];
                    int right = curEnds[r + 1];
                    int i = left, j = mid, k = left;
                    while (i < mid && j < right)
                        temp[k++] = (cmp(sorted[i], sorted[j]) <= 0)
                                    ? sorted[i++] : sorted[j++];
                    while (i < mid)   temp[k++] = sorted[i++];
                    while (j < right) temp[k++] = sorted[j++];
                    memcpy(sorted + left, temp + left,
                           (right - left) * sizeof(int));
                    newEnds[newRuns++] = right;
                    runStart = right;
                }
            }
            free(curEnds);
            curEnds = newEnds;
            curRuns = newRuns;
        }
        free(curEnds);
    }
    free(temp);

    memcpy(indexes, sorted, count * sizeof(int));
    free(sorted);
    free(runEnds);
}

void rssCountries(int *indexes, int count) {
    printf("\n[RSS] Sorting %d countries (buffer=%d)...\n", count, RSS_MEM_SIZE);
    rssGeneric(indexes, count, cmpCountryFn);
}

void rssCities(int *indexes, int count) {
    rssGeneric(indexes, count, cmpCityFn);
}

void rssProducts(int *indexes, int count) {
    rssGeneric(indexes, count, cmpProductFn);
}


void createAllFilesAndIndexes(cJSON *root) {
    FILE *datFile = fopen("products.dat", "wb");
    if (datFile == NULL) {
        printf("products.dat could not be created!\n");
        return;
    }

    int jsonCountryCount = cJSON_GetArraySize(root);

    for (int i = 0; i < jsonCountryCount; i++) {
        cJSON *countryObj  = cJSON_GetArrayItem(root, i);
        cJSON *countryName = cJSON_GetObjectItem(countryObj, "country");
        cJSON *cities      = cJSON_GetObjectItem(countryObj, "cities");

        if (countryObj == NULL || cities == NULL || !cJSON_IsArray(cities)) continue;
        if (countryCountGlobal >= MAX_COUNTRIES) {
            printf("Country index limit exceeded.\n");
            break;
        }

        safeCopy(countryIndex[countryCountGlobal].country,
                 countryName,
                 sizeof(countryIndex[countryCountGlobal].country));

        int cityStart      = cityCountGlobal;
        int localCityCount = cJSON_GetArraySize(cities);

        for (int j = 0; j < localCityCount; j++) {
            if (cityCountGlobal >= MAX_CITIES) {
                printf("City index limit exceeded.\n");
                break;
            }

            cJSON *cityObj  = cJSON_GetArrayItem(cities, j);
            cJSON *cityName = cJSON_GetObjectItem(cityObj, "city_name");
            cJSON *products = cJSON_GetObjectItem(cityObj, "products");

            int currentCityIdx = cityCountGlobal;

            safeCopy(cityIndex[currentCityIdx].country,
                     countryName, sizeof(cityIndex[currentCityIdx].country));
            safeCopy(cityIndex[currentCityIdx].city,
                     cityName,    sizeof(cityIndex[currentCityIdx].city));
            cityIndex[currentCityIdx].next_city = -1;

            int productStart      = productCountGlobal;
            int localProductCount = 0;

            if (products != NULL && cJSON_IsArray(products))
                localProductCount = cJSON_GetArraySize(products);

            for (int k = 0; k < localProductCount; k++) {
                if (productCountGlobal >= MAX_PRODUCTS) {
                    printf("Product index limit exceeded.\n");
                    break;
                }

                cJSON *productObj = cJSON_GetArrayItem(products, k);

                ProductRecord record;
                memset(&record, 0, sizeof(ProductRecord));

                safeCopy(record.country, countryName, sizeof(record.country));
                safeCopy(record.city,    cityName,    sizeof(record.city));
                safeCopy(record.product_id,
                         cJSON_GetObjectItem(productObj, "product_id"),
                         sizeof(record.product_id));

                cJSON *productInfo = cJSON_GetObjectItem(productObj, "product_info");
                if (productInfo != NULL && cJSON_IsObject(productInfo)) {
                    safeCopy(record.product_name,
                             cJSON_GetObjectItem(productInfo, "name"),
                             sizeof(record.product_name));
                    safeCopy(record.brand,
                             cJSON_GetObjectItem(productInfo, "brand"),
                             sizeof(record.brand));
                    safeCopy(record.category,
                             cJSON_GetObjectItem(productInfo, "category"),
                             sizeof(record.category));
                }

                cJSON *pricing = cJSON_GetObjectItem(productObj, "pricing");
                if (pricing != NULL && cJSON_IsObject(pricing)) {
                    cJSON *price = cJSON_GetObjectItem(pricing, "price");
                    if (price != NULL && cJSON_IsNumber(price))
                        record.price = price->valuedouble;
                    safeCopy(record.currency,
                             cJSON_GetObjectItem(pricing, "currency"),
                             sizeof(record.currency));
                }

                cJSON *inventory = cJSON_GetObjectItem(productObj, "inventory");
                if (inventory != NULL && cJSON_IsObject(inventory)) {
                    cJSON *stock = cJSON_GetObjectItem(inventory, "stock");
                    if (stock != NULL && cJSON_IsNumber(stock))
                        record.stock = stock->valueint;
                    safeCopy(record.warehouse,
                             cJSON_GetObjectItem(inventory, "warehouse"),
                             sizeof(record.warehouse));
                }

                safeCopy(record.isbn,        cJSON_GetObjectItem(productObj, "isbn"),        sizeof(record.isbn));
                safeCopy(record.description, cJSON_GetObjectItem(productObj, "description"), sizeof(record.description));
                safeCopy(record.extra,       cJSON_GetObjectItem(productObj, "extra"),       sizeof(record.extra));

                long offset = ftell(datFile);
                fwrite(&record, sizeof(ProductRecord), 1, datFile);

                int curProd = productCountGlobal;
                safeCopy(productIndex[curProd].country,
                         countryName, sizeof(productIndex[curProd].country));
                safeCopy(productIndex[curProd].city,
                         cityName,    sizeof(productIndex[curProd].city));
                strcpy(productIndex[curProd].product_name, record.product_name);
                productIndex[curProd].next_product = -1;
                productIndex[curProd].dat_offset   = offset;

                productCountGlobal++;
            } /* end product loop */

            /* --- Sort products for this city using RSS (FIX #3) --- */
            int *productOrder = malloc(localProductCount * sizeof(int));
            if (!productOrder) { printf("malloc failed\n"); fclose(datFile); return; }
            for (int p = 0; p < localProductCount; p++)
                productOrder[p] = productStart + p;

            rssProducts(productOrder, localProductCount);

            /* Wire next_product pointers */
            for (int p = 0; p < localProductCount; p++) {
                productIndex[productOrder[p]].next_product =
                    (p == localProductCount - 1) ? -1 : productOrder[p + 1];
            }
            cityIndex[currentCityIdx].product_offset =
                (localProductCount > 0) ? productOrder[0] : -1;

            free(productOrder);
            cityCountGlobal++;
        } /* end city loop */

        /* --- Sort cities for this country using RSS (FIX #2) --- */
        int *cityOrder = malloc(localCityCount * sizeof(int));
        if (!cityOrder) { printf("malloc failed\n"); fclose(datFile); return; }
        for (int c = 0; c < localCityCount; c++)
            cityOrder[c] = cityStart + c;

        rssCities(cityOrder, localCityCount);

        /* Wire next_city pointers */
        for (int c = 0; c < localCityCount; c++) {
            cityIndex[cityOrder[c]].next_city =
                (c == localCityCount - 1) ? -1 : cityOrder[c + 1];
        }
        countryIndex[countryCountGlobal].city_offset =
            (localCityCount > 0) ? cityOrder[0] : -1;

        free(cityOrder);
        countryCountGlobal++;
    } /* end country loop */

    fclose(datFile);

    /* Save original insertion order for demo */
    for (int i = 0; i < countryCountGlobal; i++)
        originalCountryInput[i] = countryIndex[i];

    /* --- Sort countries using RSS (FIX #1) --- */
    int *countryOrder = malloc(countryCountGlobal * sizeof(int));
    if (!countryOrder) { printf("malloc failed\n"); return; }
    for (int i = 0; i < countryCountGlobal; i++) countryOrder[i] = i;

    rssCountries(countryOrder, countryCountGlobal);

    /* Re-order countryIndex in place */
    CountryIndex *tempCI = malloc(countryCountGlobal * sizeof(CountryIndex));
    if (!tempCI) { free(countryOrder); return; }
    for (int i = 0; i < countryCountGlobal; i++)
        tempCI[i] = countryIndex[countryOrder[i]];
    for (int i = 0; i < countryCountGlobal; i++)
        countryIndex[i] = tempCI[i];
    free(tempCI);
    free(countryOrder);

    /* Write index files */
    FILE *countryFile = fopen("country_index.dat", "wb");
    fwrite(countryIndex, sizeof(CountryIndex), countryCountGlobal, countryFile);
    fclose(countryFile);

    FILE *cityFile = fopen("city_index.dat", "wb");
    fwrite(cityIndex, sizeof(CityIndex), cityCountGlobal, cityFile);
    fclose(cityFile);

    FILE *productFile = fopen("product_index.dat", "wb");
    fwrite(productIndex, sizeof(ProductIndex), productCountGlobal, productFile);
    fclose(productFile);

    printf("products.dat created successfully.\n");
    printf("country_index.dat created successfully.\n");
    printf("city_index.dat created successfully.\n");
    printf("product_index.dat created successfully.\n");
    printf("\nTotal countries : %d\n", countryCountGlobal);
    printf("Total cities    : %d\n", cityCountGlobal);
    printf("Total products  : %d\n", productCountGlobal);
}


void saveUpdatedIndexes(void) {
    FILE *countryFile = fopen("country_index.dat", "wb");
    fwrite(countryIndex, sizeof(CountryIndex), countryCountGlobal, countryFile);
    fclose(countryFile);

    FILE *cityFile = fopen("city_index.dat", "wb");
    fwrite(cityIndex, sizeof(CityIndex), cityCountGlobal, cityFile);
    fclose(cityFile);

    FILE *productFile = fopen("product_index.dat", "wb");
    fwrite(productIndex, sizeof(ProductIndex), productCountGlobal, productFile);
    fclose(productFile);
}



void displayCountryIndex(void) {
    printf("\n===== COUNTRY INDEX =====\n");
    for (int i = 0; i < countryCountGlobal; i++) {
        printf("[%d] Country: %-15s | First City Offset: %d\n",
               i, countryIndex[i].country, countryIndex[i].city_offset);
    }
}

void displayCityIndex(void) {
    printf("\n===== CITY INDEX - LINKED LIST TRAVERSAL =====\n");
    for (int i = 0; i < countryCountGlobal; i++) {
        printf("\nCountry: %s\n", countryIndex[i].country);
        int cityPtr = countryIndex[i].city_offset;
        while (cityPtr != -1) {
            printf("  City Index [%d] | City: %-15s | Next City: %d | Product Offset: %d\n",
                   cityPtr,
                   cityIndex[cityPtr].city,
                   cityIndex[cityPtr].next_city,
                   cityIndex[cityPtr].product_offset);
            cityPtr = cityIndex[cityPtr].next_city;
        }
    }
}

void displayProductIndex(void) {
    printf("\n===== PRODUCT INDEX - LINKED LIST TRAVERSAL =====\n");
    for (int i = 0; i < countryCountGlobal; i++) {
        int cityPtr = countryIndex[i].city_offset;
        while (cityPtr != -1) {
            printf("\nCountry: %s | City: %s\n",
                   cityIndex[cityPtr].country, cityIndex[cityPtr].city);
            int productPtr = cityIndex[cityPtr].product_offset;
            while (productPtr != -1) {
                printf("  Product Index [%d] | Product: %-25s | Next: %d | .dat Offset: %ld\n",
                       productPtr,
                       productIndex[productPtr].product_name,
                       productIndex[productPtr].next_product,
                       productIndex[productPtr].dat_offset);
                productPtr = productIndex[productPtr].next_product;
            }
            cityPtr = cityIndex[cityPtr].next_city;
        }
    }
}

void displaySortedIndexLevels(void) {
    int choice;
    printf("\n===== SORT AND DISPLAY INDEX LEVELS =====\n");
    printf("1. Country Index\n");
    printf("2. City Index\n");
    printf("3. Product Index\n");
    printf("Choose index level: ");
    scanf("%d", &choice);
    getchar();

    if      (choice == 1) displayCountryIndex();
    else if (choice == 2) displayCityIndex();
    else if (choice == 3) displayProductIndex();
    else                  printf("Invalid index level.\n");
}


int findCountry(char countryName[]) {
    /* Binary search — valid because countryIndex[] is physically sorted */
    int lo = 0, hi = countryCountGlobal - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int cmp = strcasecmp(countryIndex[mid].country, countryName);
        if      (cmp == 0) return mid;
        else if (cmp < 0)  lo = mid + 1;
        else               hi = mid - 1;
    }
    return -1;
}

int findCityByCountryAndCity(char countryName[], char cityName[]) {
    int countryPos = findCountry(countryName);
    if (countryPos == -1) return -1;

    /* Traverse linked-list — no full scan */
    int cityPtr = countryIndex[countryPos].city_offset;
    while (cityPtr != -1) {
        if (strcasecmp(cityIndex[cityPtr].city, cityName) == 0) return cityPtr;
        cityPtr = cityIndex[cityPtr].next_city;
    }
    return -1;
}

void searchProduct(char countryName[], char cityName[], char productName[]) {
    printf("\n===== SEARCH PATH =====\n");

    int countryPos = findCountry(countryName);
    if (countryPos == -1) { printf("Country not found.\n"); return; }

    printf("Country Index [%d] found: %s\n",
           countryPos, countryIndex[countryPos].country);

    int cityPtr = countryIndex[countryPos].city_offset;
    while (cityPtr != -1) {
        printf("  City Index [%d] visited: %s\n", cityPtr, cityIndex[cityPtr].city);

        if (strcasecmp(cityIndex[cityPtr].city, cityName) == 0) {
            printf("  City found: %s\n", cityIndex[cityPtr].city);

            int productPtr = cityIndex[cityPtr].product_offset;
            while (productPtr != -1) {
                printf("    Product Index [%d] visited: %s\n",
                       productPtr, productIndex[productPtr].product_name);

                if (strcasecmp(productIndex[productPtr].product_name, productName) == 0) {
                    printf("    Product found: %s\n",
                           productIndex[productPtr].product_name);

                    FILE *file = fopen("products.dat", "rb");
                    if (file == NULL) {
                        printf("products.dat could not be opened.\n");
                        return;
                    }

                    ProductRecord record;
                    fseek(file, productIndex[productPtr].dat_offset, SEEK_SET);
                    fread(&record, sizeof(ProductRecord), 1, file);
                    fclose(file);

                    printf("\n===== DATA FILE RECORD =====\n");
                    printf("Product ID   : %s\n", record.product_id);
                    printf("Product Name : %s\n", record.product_name);
                    printf("Brand        : %s\n", record.brand);
                    printf("Category     : %s\n", record.category);
                    printf("Country      : %s\n", record.country);
                    printf("City         : %s\n", record.city);
                    printf("Price        : %.2lf %s\n", record.price, record.currency);
                    printf("Stock        : %d\n", record.stock);
                    printf("Warehouse    : %s\n", record.warehouse);
                    return;
                }
                productPtr = productIndex[productPtr].next_product;
            }
            printf("Product not found in this city.\n");
            return;
        }
        cityPtr = cityIndex[cityPtr].next_city;
    }
    printf("City not found in this country.\n");
}


void searchByCountry(void) {
    char country[50];
    printf("Country: ");
    readLine(country, sizeof(country));

    int countryPos = findCountry(country);
    if (countryPos == -1) { printf("Country not found.\n"); return; }

    printf("\nCountry found in Country Index [%d]\n", countryPos);
    printf("Country          : %s\n", countryIndex[countryPos].country);
    printf("First City Offset: %d\n", countryIndex[countryPos].city_offset);

    FILE *datFile = fopen("products.dat", "rb");
    if (datFile == NULL) { printf("products.dat could not be opened.\n"); return; }

    printf("\n--- Cities & Products (alphabetical by city, then product) ---\n");

    int cityPtr = countryIndex[countryPos].city_offset;
    while (cityPtr != -1) {
        printf("\n  City Index [%d] -> %s\n", cityPtr, cityIndex[cityPtr].city);

        int productPtr = cityIndex[cityPtr].product_offset;
        while (productPtr != -1) {
            printf("    Product Index [%d] -> %s  (.dat offset: %ld)\n",
                   productPtr,
                   productIndex[productPtr].product_name,
                   productIndex[productPtr].dat_offset);

            /* Read actual record from .dat using stored offset (no file scan) */
            ProductRecord record;
            fseek(datFile, productIndex[productPtr].dat_offset, SEEK_SET);
            fread(&record, sizeof(ProductRecord), 1, datFile);
            printf("      ID: %-12s | Brand: %-15s | Price: %.2lf %-5s | Stock: %d\n",
                   record.product_id, record.brand,
                   record.price, record.currency, record.stock);

            productPtr = productIndex[productPtr].next_product;
        }
        cityPtr = cityIndex[cityPtr].next_city;
    }
    fclose(datFile);
}

void searchByCity(void) {
    char city[50];
    printf("City: ");
    readLine(city, sizeof(city));

    int found = 0;
    /* Traverse all countries via index, then follow city linked lists */
    for (int i = 0; i < countryCountGlobal; i++) {
        int cityPtr = countryIndex[i].city_offset;
        while (cityPtr != -1) {
            if (strcasecmp(cityIndex[cityPtr].city, city) == 0) {
                found = 1;
                printf("\nCity found in City Index [%d]\n", cityPtr);
                printf("Country              : %s\n", cityIndex[cityPtr].country);
                printf("City                 : %s\n", cityIndex[cityPtr].city);
                printf("First Product Offset : %d\n", cityIndex[cityPtr].product_offset);

                printf("\nProducts (alphabetical order via pointer traversal):\n");
                int productPtr = cityIndex[cityPtr].product_offset;
                while (productPtr != -1) {
                    printf("  Product Index [%d] -> %s\n",
                           productPtr, productIndex[productPtr].product_name);
                    productPtr = productIndex[productPtr].next_product;
                }
            }
            cityPtr = cityIndex[cityPtr].next_city;
        }
    }
    if (!found) printf("City not found.\n");
}

void searchByProductName(void) {
    char product[100];
    printf("Product Name: ");
    readLine(product, sizeof(product));

    int found = 0;

    printf("\n===== SEARCH BY PRODUCT NAME (SUMMARY) =====\n");
    printf("Searching '%s' through Country -> City -> Product index pointers.\n", product);
    printf("Only matching records are displayed to keep the output readable.\n\n");

    FILE *file = fopen("products.dat", "rb");
    if (file == NULL) {
        printf("products.dat could not be opened.\n");
        return;
    }

    /* Traverse Country -> City -> Product via index pointers only */
    for (int i = 0; i < countryCountGlobal; i++) {
        int cityPtr = countryIndex[i].city_offset;

        while (cityPtr != -1) {
            int productPtr = cityIndex[cityPtr].product_offset;

            while (productPtr != -1) {
                if (strcasecmp(productIndex[productPtr].product_name, product) == 0) {
                    ProductRecord record;

                    fseek(file, productIndex[productPtr].dat_offset, SEEK_SET);
                    fread(&record, sizeof(ProductRecord), 1, file);

                    found++;

                    printf("%2d) Country: %-12s | City: %-12s | Product Index: %-4d | .dat Offset: %-6ld | ID: %-15s | Price: %.2lf %s | Stock: %d\n",
                           found,
                           record.country,
                           record.city,
                           productPtr,
                           productIndex[productPtr].dat_offset,
                           record.product_id,
                           record.price,
                           record.currency,
                           record.stock);
                }

                productPtr = productIndex[productPtr].next_product;
            }

            cityPtr = cityIndex[cityPtr].next_city;
        }
    }

    fclose(file);

    if (found == 0) {
        printf("Product '%s' not found in any Country/City/Product traversal path.\n", product);
    } else {
        printf("\nTotal matches found: %d\n", found);
        printf("Note: Use menu option 4 for a single exact record search with full details.\n");
    }
}



void insertProduct(void) {
    ProductRecord record;
    memset(&record, 0, sizeof(ProductRecord));

    printf("\n===== INSERT A NEW PRODUCT =====\n");
    printf("Country     : ");  readLine(record.country,      sizeof(record.country));
    printf("City        : ");  readLine(record.city,         sizeof(record.city));

    int cityPos = findCityByCountryAndCity(record.country, record.city);
    if (cityPos == -1) { printf("Country or city not found. Insert cancelled.\n"); return; }

    printf("Product ID  : ");  readLine(record.product_id,   sizeof(record.product_id));
    printf("Product Name: ");  readLine(record.product_name, sizeof(record.product_name));
    printf("Brand       : ");  readLine(record.brand,        sizeof(record.brand));
    printf("Category    : ");  readLine(record.category,     sizeof(record.category));
    printf("Price       : ");  scanf("%lf", &record.price);  getchar();
    printf("Currency    : ");  readLine(record.currency,     sizeof(record.currency));
    printf("Stock       : ");  scanf("%d",  &record.stock);  getchar();
    printf("Warehouse   : ");  readLine(record.warehouse,    sizeof(record.warehouse));
    printf("ISBN        : ");  readLine(record.isbn,         sizeof(record.isbn));
    printf("Description : ");  readLine(record.description,  sizeof(record.description));
    printf("Extra       : ");  readLine(record.extra,        sizeof(record.extra));

    /* FIX #6: open for read+append; use fseek to get reliable end offset */
    FILE *file = fopen("products.dat", "r+b");
    if (file == NULL) { printf("products.dat could not be opened.\n"); return; }
    fseek(file, 0, SEEK_END);
    long datOffset = ftell(file);
    fwrite(&record, sizeof(ProductRecord), 1, file);
    fclose(file);

    /* Append to product index array */
    int newIndex = productCountGlobal;
    if (newIndex >= MAX_PRODUCTS) { printf("Product index full.\n"); return; }

    strcpy(productIndex[newIndex].country,      record.country);
    strcpy(productIndex[newIndex].city,         record.city);
    strcpy(productIndex[newIndex].product_name, record.product_name);
    productIndex[newIndex].dat_offset   = datOffset;
    productIndex[newIndex].next_product = -1;

    /* Find correct insertion point in the alphabetically-linked list */
    int oldHead  = cityIndex[cityPos].product_offset;
    int previous = -1;
    int current  = oldHead;

    printf("\n===== POINTER UPDATE PATH =====\n");
    printf("Target City Index [%d]: %s / %s\n",
           cityPos, cityIndex[cityPos].country, cityIndex[cityPos].city);
    printf("Old Product Offset: %d\n", oldHead);
    printf("New Product Index : %d\n", newIndex);
    printf("New .dat Offset   : %ld\n", datOffset);

    while (current != -1 &&
           strcasecmp(productIndex[current].product_name, record.product_name) < 0) {
        printf("  Visited Product Index [%d]: %s\n",
               current, productIndex[current].product_name);
        previous = current;
        current  = productIndex[current].next_product;
    }

    if (previous == -1) {
        /* New product is alphabetically first */
        productIndex[newIndex].next_product = oldHead;
        cityIndex[cityPos].product_offset   = newIndex;
        printf("\nUpdate: City Index [%d].product_offset: %d -> %d\n",
               cityPos, oldHead, newIndex);
        printf("        Product Index [%d].next_product = %d\n", newIndex, oldHead);
    } else {
        int oldNext = productIndex[previous].next_product;
        productIndex[newIndex].next_product    = oldNext;
        productIndex[previous].next_product    = newIndex;
        printf("\nUpdate: Product Index [%d].next_product: %d -> %d\n",
               previous, oldNext, newIndex);
        printf("        Product Index [%d].next_product = %d\n", newIndex, oldNext);
    }

    productCountGlobal++;
    saveUpdatedIndexes();

    printf("\nProduct inserted successfully.\n");
    printf("\nNew logical product order for city '%s':\n", cityIndex[cityPos].city);
    int ptr = cityIndex[cityPos].product_offset;
    while (ptr != -1) {
        printf("  Product Index [%d] -> %s%s\n",
               ptr,
               productIndex[ptr].product_name,
               (productIndex[ptr].next_product == -1) ? " (END)" : "");
        ptr = productIndex[ptr].next_product;
    }
}


void applyReplacementSelectionSortDemo(void) {
    const int MEM_SIZE = RSS_MEM_SIZE;

    CountryIndex memory[RSS_MEM_SIZE];
    int frozen[RSS_MEM_SIZE];
    int inputPos  = 0;
    int memCount  = 0;
    int runNumber = 1;

    printf("\n===== REPLACEMENT SELECTION SORT DEMO =====\n");
    printf("Input source : original Country Index order from JSON\n");
    printf("Memory size  : %d slots\n\n", MEM_SIZE);

    while (inputPos < countryCountGlobal || memCount > 0) {

        /* Fill memory */
        while (memCount < MEM_SIZE && inputPos < countryCountGlobal) {
            memory[memCount]  = originalCountryInput[inputPos];
            frozen[memCount]  = 0;
            printf("  Loaded into memory slot [%d]: %s\n",
                   memCount, memory[memCount].country);
            memCount++;
            inputPos++;
        }

        printf("\n--- Run %d ---\n", runNumber);
        char lastOutput[50] = "";
        int  producedInRun  = 0;

        while (1) {
            /* Find minimum non-frozen element */
            int minIdx = -1;
            for (int i = 0; i < memCount; i++) {
                if (!frozen[i]) {
                    if (minIdx == -1 ||
                        strcasecmp(memory[i].country, memory[minIdx].country) < 0) {
                        minIdx = i;
                    }
                }
            }
            if (minIdx == -1) break;  /* all slots frozen */

            printf("  Output : %-20s (city_offset=%d)\n",
                   memory[minIdx].country, memory[minIdx].city_offset);
            strcpy(lastOutput, memory[minIdx].country);
            producedInRun = 1;

            if (inputPos < countryCountGlobal) {
                CountryIndex next = originalCountryInput[inputPos++];
                printf("  Read   : %s  ", next.country);
                memory[minIdx] = next;
                if (strcasecmp(next.country, lastOutput) < 0) {
                    frozen[minIdx] = 1;
                    printf("-> FROZEN (< last output)\n");
                } else {
                    frozen[minIdx] = 0;
                    printf("-> active\n");
                }
            } else {
                /* Compact: remove the slot */
                for (int j = minIdx; j < memCount - 1; j++) {
                    memory[j] = memory[j + 1];
                    frozen[j] = frozen[j + 1];
                }
                memCount--;
            }
        }

        /* Thaw all for next run */
        for (int i = 0; i < memCount; i++) frozen[i] = 0;

        if (producedInRun) {
            printf("--- End of Run %d ---\n\n", runNumber++);
        }
    }

    printf("Replacement Selection Sort demo completed.\n");
}


int main(void) {
    char *jsonText = readFile("Assignment -2.json");
    if (jsonText == NULL) return 1;

    cJSON *root = cJSON_Parse(jsonText);
    if (root == NULL) {
        printf("JSON parse error.\n");
        free(jsonText);
        return 1;
    }

    printf("JSON parsed successfully.\n\n");
    createAllFilesAndIndexes(root);

    int  choice;
    char country[50], city[50], product[100];

    do {
        printf("\n========== MULTI-LEVEL INDEXING MENU ==========\n");
        printf("1. Search by Country (lists all cities & products)\n");
        printf("2. Search by City (lists all products)\n");
        printf("3. Search by Product Name\n");
        printf("4. Search by Country / City / Product (exact record)\n");
        printf("5. Sort and Display Index Levels\n");
        printf("6. Insert a New Product\n");
        printf("7. Apply Replacement Selection Sort Demo\n");
        printf("8. Exit\n");
        printf("Choose: ");

        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1:
                searchByCountry();
                break;
            case 2:
                searchByCity();
                break;
            case 3:
                searchByProductName();
                break;
            case 4:
                printf("Country     : ");  readLine(country, sizeof(country));
                printf("City        : ");  readLine(city,    sizeof(city));
                printf("Product Name: ");  readLine(product, sizeof(product));
                searchProduct(country, city, product);
                break;
            case 5:
                displaySortedIndexLevels();
                break;
            case 6:
                insertProduct();
                break;
            case 7:
                applyReplacementSelectionSortDemo();
                break;
            case 8:
                printf("Exiting. Files closed and memory freed.\n");
                break;
            default:
                printf("Invalid choice.\n");
        }

    } while (choice != 8);

    cJSON_Delete(root);
    free(jsonText);
    return 0;
}