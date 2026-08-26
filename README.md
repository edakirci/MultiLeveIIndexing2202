# Multi-Level Indexing System

![C](https://img.shields.io/badge/C-Programming-A8B9CC?logo=c&logoColor=white)
![JSON](https://img.shields.io/badge/Data-JSON-000000?logo=json&logoColor=white)
![Docker](https://img.shields.io/badge/Environment-Docker-2496ED?logo=docker&logoColor=white)
![cJSON](https://img.shields.io/badge/Library-cJSON-orange)

This project is a **console-based multi-level secondary indexing system developed in C**.

The application reads a structured retail dataset from JSON, stores product records in a binary data file, and creates a hierarchical indexing system based on **Country, City, and Product**.

The indexes are connected using offset-based linked structures, allowing records to be accessed through the following path:

```text
Country Index → City Index → Product Index → Binary Data Record
```

Replacement Selection Sort is used to maintain alphabetical ordering across the index levels. The application also provides an interactive menu for searching, displaying indexes, inserting new products, and observing the sorting process.

---

## Features

- Parse a structured JSON retail dataset using cJSON
- Convert JSON product records into a binary `.dat` file
- Build Country, City, and Product secondary indexes
- Sort index entries using Replacement Selection Sort
- Store cities and products as logically sorted linked structures
- Locate countries using binary search
- Traverse records through offset and next-pointer values
- Retrieve product records directly from the binary data file
- Search by country, city, or product name
- Perform an exact Country → City → Product lookup
- Display Country, City, and Product index levels
- Insert new products without physically reorganizing existing records
- Recalculate the affected index pointers after insertion
- Demonstrate Replacement Selection Sort step by step
- Continuously display an interactive menu until the user exits

---

## Index Architecture

The application uses three secondary index levels and one physical data level.

| Level | Structure | Purpose |
|---:|---|---|
| 1 | Country Index | Stores countries and points to the first city of each country |
| 2 | City Index | Stores cities and points to the next city and first product |
| 3 | Product Index | Stores product names and points to the next product and binary record |
| 4 | `products.dat` | Stores the complete product records in binary format |

### Logical Traversal

```text
Country
  └── First City Offset
        └── Next City Offset
              └── First Product Offset
                    └── Next Product Offset
                          └── products.dat Offset
```

The Country Index is physically sorted alphabetically. City and Product entries may remain physically scattered, but their pointer values preserve alphabetical traversal.

---

## Index Structures

### Country Index

Each country entry contains:

| Field | Description |
|---|---|
| `country` | Country name |
| `city_offset` | Index of the country's first city |

Countries are physically sorted in alphabetical order using Replacement Selection Sort.

### City Index

Each city entry contains:

| Field | Description |
|---|---|
| `country` | Country containing the city |
| `city` | City name |
| `next_city` | Index of the next city in alphabetical order |
| `product_offset` | Index of the city's first product |

The value `-1` indicates the end of the city list.

### Product Index

Each product entry contains:

| Field | Description |
|---|---|
| `country` | Product's country |
| `city` | Product's city |
| `product_name` | Product name |
| `next_product` | Index of the next product in alphabetical order |
| `dat_offset` | Byte offset of the complete record in `products.dat` |

The value `-1` indicates the end of the product list.

---

## Product Record Fields

Each binary product record contains the following information:

| Field | Description |
|---|---|
| `product_id` | Unique product identifier |
| `product_name` | Product name |
| `brand` | Product brand |
| `category` | Product category |
| `country` | Country where the product is available |
| `city` | City where the product is available |
| `price` | Product price |
| `currency` | Price currency |
| `stock` | Available stock quantity |
| `warehouse` | Warehouse information |
| `isbn` | ISBN value when applicable |
| `description` | Product description |
| `extra` | Additional product information |

---

## Replacement Selection Sort

Replacement Selection Sort is applied to:

- Countries
- Cities within each country
- Products within each city

The program uses a memory buffer of three elements:

```c
#define RSS_MEM_SIZE 3
```

The sorting process consists of two stages:

1. Sorted runs are produced using active and frozen memory elements.
2. The generated runs are merged until a single sorted sequence remains.

The interactive demonstration displays:

- Items loaded into memory
- Selected output items
- Active and frozen items
- The end of each generated run

---

## Search Operations

### Search by Country

The application uses binary search on the sorted Country Index. It then follows the linked City and Product indexes to list all available products alphabetically by city and product name.

### Search by City

The application traverses the Country and City indexes and displays the products connected to the matching city.

### Search by Product Name

The Country → City → Product index hierarchy is traversed, and matching records are retrieved from `products.dat` using their stored byte offsets.

### Exact Record Search

An exact search follows this path:

```text
Country → City → Product → products.dat
```

The application displays every visited index entry and retrieves the corresponding record using `fseek()` and the stored `.dat` offset.

---

## Product Insertion

A new product is appended to the end of `products.dat` without physically moving the existing records.

The program then:

1. Creates a new Product Index entry.
2. Locates the correct alphabetical position.
3. Updates the previous product's `next_product` pointer.
4. Updates the city's `product_offset` when the new product becomes the first item.
5. Saves the updated index files.

This preserves the logical alphabetical order even though the physical records remain in insertion order.

---

## Interactive Menu

When the application starts, the following menu is displayed:

```text
========== MULTI-LEVEL INDEXING MENU ==========
1. Search by Country (lists all cities & products)
2. Search by City (lists all products)
3. Search by Product Name
4. Search by Country / City / Product (exact record)
5. Sort and Display Index Levels
6. Insert a New Product
7. Apply Replacement Selection Sort Demo
8. Exit
```

The program continues running until option `8` is selected.

---

## Technologies Used

- C
- cJSON
- JSON parsing
- Binary file processing
- Multi-level secondary indexing
- Linked index structures
- Offset-based file access
- Replacement Selection Sort
- Binary search
- Docker
- GCC

---

## System Requirements

The recommended method is to run the project using Docker.

Required software:

- Docker Desktop
- Git
- Visual Studio Code or another text editor

For native Linux compilation, the following packages are required:

- GCC
- cJSON development library

---

## Project Structure

```text
MultiLevelIndexing2202/
├── 2023510239.c        # Main C source code
├── Assignment -2.json  # Retail product dataset
├── Dockerfile          # Docker development environment
├── .gitignore          # Ignored generated and compiled files
└── README.md           # Project documentation
```

The following files are generated when the application runs and are not stored in the repository:

```text
homework
products.dat
country_index.dat
city_index.dat
product_index.dat
```

---

## Generated Files

| File | Description |
|---|---|
| `products.dat` | Complete product records in binary format |
| `country_index.dat` | Sorted Country Index entries |
| `city_index.dat` | City Index entries and linked offsets |
| `product_index.dat` | Product Index entries and binary record offsets |

These files are recreated from `Assignment -2.json` when the application starts.

---

## How to Run

### 1. Clone the Repository

```bash
git clone https://github.com/edakirci/MultiLevelIndexing2202.git
cd MultiLevelIndexing2202
```

### 2. Build the Docker Image

Open PowerShell in the project directory and run:

```powershell
docker build -t multilevel-index-env .
```

This creates a Docker environment containing:

- GCC
- cJSON development library
- Valgrind

### 3. Start the Docker Container

In PowerShell, run:

```powershell
docker run -it --rm -v "${PWD}:/app" multilevel-index-env
```

The project directory is mounted at `/app` inside the container.

### 4. Open the Project Directory

Inside the Docker container, run:

```bash
cd /app
ls
```

Make sure the following files are visible:

```text
2023510239.c
Assignment -2.json
Dockerfile
README.md
```

### 5. Compile the Application

```bash
gcc 2023510239.c -o homework -lcjson
```

### 6. Run the Application

```bash
./homework
```

After startup, the JSON dataset is parsed and the following binary files are generated automatically:

```text
products.dat
country_index.dat
city_index.dat
product_index.dat
```

The interactive menu is then displayed.

---

## Running with Valgrind

Valgrind is included in the Docker environment and can be used to check for memory-related problems:

```bash
valgrind --leak-check=full ./homework
```

For a more detailed report:

```bash
valgrind --leak-check=full --show-leak-kinds=all ./homework
```

---

## Example Startup Output

```text
JSON parsed successfully.

[RSS] Sorting countries...
products.dat created successfully.
country_index.dat created successfully.
city_index.dat created successfully.
product_index.dat created successfully.

Total countries : 10
Total cities    : 80
Total products  : 800
```

The exact totals depend on the contents of the supplied JSON dataset.

---

## Important Notes

- The JSON file must be named exactly `Assignment -2.json`.
- The application searches for this filename when it starts.
- Run the program from the directory containing the JSON file.
- Generated `.dat` files are excluded from Git using `.gitignore`.
- Product insertion updates the generated binary and index files during the active run.
- Starting the application again recreates the generated files from the original JSON dataset.
- Search operations use index structures and stored offsets instead of linearly scanning `products.dat`.

---

## Course Information

**Course:** CME2202 - Data Organization and Management 
**Assignment:**  Multi-Level Indexing  
**Semester:** Spring 2025–2026  
**Instructor:** Göksu Tüysüz

---
