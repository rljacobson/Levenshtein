# Function Documentation and Examples

## Usage

### DAMLEV

```sql
SELECT DAMLEV(String1, String2);
```

|    Argument | Meaning                                                      |
| ----------: | :----------------------------------------------------------- |
|   `String1` | A string                                                     |
|   `String2` | A string which will be compared to `String1`.                |
| **Returns** | Either an integer equal to the edit distance between `String1` and `String2` or `PosInt`, whichever is smaller. |

**Example Usage:**

```sql
SELECT Name, DAMLEV(Name, "Vladimir Iosifovich Levenshtein") AS EditDist 
FROM CUSTOMERS WHERE DAMLEV(Name, "Vladimir Iosifovich Levenshtein") < 15;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 15 of "Vladimir Iosifovich Levenshtein".

### DAMLEVLIM

```sql
SELECT DAMLEVLIM(String1, String2, PosInt);
```

|    Argument | Meaning                                                      |
| ----------: | :----------------------------------------------------------- |
|   `String1` | A string                                                     |
|   `String2` | A string which will be compared to `String1`.                |
| **Returns** | An integer equal to the edit distance between `String1` and `String2` or `PosInt`, whichever is smaller. |

**Example Usage:**

```sql
SELECT Name, DAMLEVLIM(Name, "Vladimir Iosifovich Levenshtein", 6) AS EditDist 
FROM CUSTOMERS WHERE DAMLEVLIM(Name, "Vladimir Iosifovich Levenshtein", 6) < 6;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 6 of "Vladimir Iosifovich Levenshtein".

### DAMLEVP

```sql
DAMLEVP(String1, String2);
```

|    Argument | Meaning                                                      |
| ----------: | :----------------------------------------------------------- |
|   `String1` | A string                                                     |
|   `String2` | A string which will be compared to `String1`.                |
| **Returns** | A floating point number in the range \[0, 1\] equal to the normalized similarity `similarity = 1.0 - edit_distance / max_string_length;` between `String1` and `String2`. This function is functionally equivalent to `1 - DAMLEV(String1, String2) / MAX(LENGTH(String1), LENGTH(String2))` but is much faster. |


**Example Usage:**

```sql
SELECT Name, DAMLEVP(Name, "Vladimir Iosifovich Levenshtein") AS EditDist 
FROM CUSTOMERS WHERE DAMLEVP(Name, "Vladimir Iosifovich Levenshtein") < 0.2;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 20% of "Vladimir Iosifovich Levenshtein".

### DAMLEVCONST

```sql
DAMLEVCONST(String1, ConstString, PosInt);
```

|      Argument | Meaning                                                      |
| ------------: | :----------------------------------------------------------- |
|     `String1` | A string                                                     |
| `ConstString` | A constant string (string literal) which will be compared to `String1`. |
|      `PosInt` | A positive integer. If the distance between `String1` and `ConstString` is greater than `PosInt`, `DAMLEVCONST()` will stop its computation at `PosInt` and return `PosInt`. Make `PosInt` as small as you can to improve speed and efficiency. For example, if you put `WHERE DAMLEVCONST(...) < k` in a `WHERE`-clause, make `PosInt` be `k`. |
|   **Returns** | Either an integer equal to the edit distance between `String1` and `ConstString` or `PosInt`, whichever is smaller. |


**Example Usage:**

```sql
SELECT Name, DAMLEVCONST(Name, "Vladimir Iosifovich Levenshtein", 8) AS EditDist 
FROM CUSTOMERS WHERE DAMLEV(Name, "Vladimir Iosifovich Levenshtein", 8) < 8;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 8 of "Vladimir Iosifovich Levenshtein".


### DAMLEV2D

This is a two row approach that is space optimized but does not calculate transpositions.
You can read about this approach here.

https://takeuforward.org/data-structure/edit-distance-dp-33/

```sql
DAMLEV2D(String1, String2);
```

|    Argument | Meaning                                                      |
| ----------: | :----------------------------------------------------------- |
|   `String1` | A string                                                     |
|   `String2` | A string which will be compared to `String1`.                |
| **Returns** | Either an integer equal to the edit distance between `String1` and `String2`. |


**Example Usage:**

```sql
SELECT Name, DAMLEV2D(Name, "Vladimir Iosifovich Levenshtein") AS EditDist 
FROM CUSTOMERS WHERE DAMLEV(Name, "Vladimir Iosifovich Levenshtein") < 8;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 8 of "Vladimir Iosifovich Levenshtein".
