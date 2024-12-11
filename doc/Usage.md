# Function Documentation, Usage, and Examples

## Levenshtein Edit Distance: `edit_dist(String1, String2)`

Computes the Levenshtein edit distance between two strings when the
edit distance is less than a given number.

Syntax:

    edit_dist(String1, String2);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.

Returns: The edit distance between `String1` and `String2`.

Example Usage:

    select Name, edit_dist(Name, "Vladimir Iosifovich Levenshtein") AS
        EditDist from Customers where  edit_dist(Name, "Vladimir Iosifovich Levenshtein") <= 6;

The above will return all rows `(Name, EditDist)` from the `Customers` table
where `Name` has edit distance within 6 of "Vladimir Iosifovich Levenshtein".


## Damarau-Levenshtein Edit Distance: `edit_dist_t(String1, String2)`

Computes the Damarau-Levenshtein edit distance (allows transpositions) between two strings.

Syntax:

    edit_dist_t(String1, String2);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.

Returns: An integer equal to the Damarau-Levenshtein edit distance between
`String1` and `String2`.

Example Usage:

    select Name, edit_dist_t(Name, "Vladimir Iosifovich Levenshtein") as EditDist
        from Customers
        where  edit_dist_t(Name, "Vladimir Iosifovich Levenshtein") <= 6;

The above will return all rows `(Name, EditDist)` from the `Customers` table
where `Name` has edit distance within 6 of "Vladimir Iosifovich Levenshtein".


## Bounded Levenshtein Edit Distance: `bounded_edit_dist(String1, String2, PosInt)`

Computes the Levenshtein edit distance between two strings when the
edit distance is less than a given number.

Syntax:

    bounded_edit_dist(String1, String2, PosInt);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.
`PosInt`:   A positive integer. If the distance between `String1` and
            `String2` is greater than `PosInt`, `bounded_edit_dist()` will stop its
            computation at `PosInt` and return `PosInt`. Make `PosInt` as
            small as you can to improve speed and efficiency. For example,
            if you put `where bounded_edit_dist(...) < k` in a `where`-clause, make
            `PosInt` be `k`.

Returns: Either an integer equal to the edit distance between `String1` and `String2` or `k`,
whichever is smaller.

Example Usage:

    select Name, bounded_edit_dist(Name, "Vladimir Iosifovich Levenshtein", 6) AS
        EditDist from Customers where  bounded_edit_dist(Name, "Vladimir Iosifovich Levenshtein", 6) <= 6;

The above will return all rows `(Name, EditDist)` from the `Customers` table
where `Name` has edit distance within 6 of "Vladimir Iosifovich Levenshtein".


## Bounded Damarau-Levenshtein Edit Distance: `bounded_edit_dist_t(String1, String2, PosInt)`

Computes the Damarau-Levenshtein edit distance between two strings when the
edit distance is less than a given number.

Syntax:

    bounded_edit_dist_t(String1, String2, PosInt);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.
`PosInt`:   A positive integer. If the distance between `String1` and
            `String2` is greater than `PosInt`, `bounded_edit_dist_t()` will stop its
            computation at `PosInt` and return `PosInt + 1`. Make `PosInt`
            as small as you can to improve speed and efficiency. For example,
            if you put `where bounded_edit_dist_t(...) <= k` in a `where`-clause, make
            `PosInt` be `k`.

Returns: Either an integer equal to the Damarau-Levenshtein edit distance between
`String1` and `String2` or `k+1`, whichever is smaller.

Example Usage:

    select Name, bounded_edit_dist_t(Name, "Vladimir Iosifovich Levenshtein", 6) AS EditDist
        from Customers
        where  bounded_edit_dist_t(Name, "Vladimir Iosifovich Levenshtein", 6) <= 6;

The above will return all rows `(Name, EditDist)` from the `Customers` table
where `Name` has edit distance within 6 of "Vladimir Iosifovich Levenshtein".


## Damarau-Levenshtein Similarity: `similarity_t(String1, String2, RealNum)`


Computes the normalized Damarau-Levenshtein edit distance between two strings.
The normalization is the edit distance divided by the length of the longest string:

    ("edit distance")/("length of longest string").

One can easily use `bounded_edit_dist_t(..)` and this formula instead to achieve the same
thing, so this can be seen as a convenience function. Note that the only variant of this
function is the bounded Damarau (transpositions) variant and its `min_` counterpart. 
For unbounded or regular Levenshtein (no transpositions), you can use the formula above 
or request for us to add it.

Syntax:

    similarity_t(String1, String2, RealNum);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.
`RealNum`:  A number between 0.0 and 1.0 inclusive. If the similarity is less than 
            `RealNum`,  `similarity_t()` will stop computation and return some number less than
            `RealNum`. Make `RealNum` as close to 1.0 as possible.

Returns: A floating point number equal to the normalized edit distance between `String1` and
`String2`.

Example Usage:

    select Name, similarity_t(Name, "Vladimir Iosifovich Levenshtein", 0.80) as
        EditDist from Customers where similarity_t(Name, "Vladimir Iosifovich Levenshtein", 0.80)  >= 0.80;

The above will return all rows `(Name, EditDist)` from the `Customers` table
where `Name` at least 80% similar to "Vladimir Iosifovich Levenshtein".


## Minimum Levenshtein Edit Distance / "Closest Match": `min_edit_dist(String1, String2, PosInt)`

Computes the Levenshtein edit distance (without transpositions) between two strings unless
that distance will exceed `current_min_distance`, the minimum edit distance
it has computed in the query so far, in which case it will return
`current_min_distance + 1`. The `current_min_distance` is initialized to
`PosInt`.

In the common case that we wish to find the rows that have the *smallest*
distance between strings, we can achieve significant performance
improvements if we stop the computation when we know the distance will be
*greater* than some other distance we have already computed.

Syntax:

    min_edit_dist(String1, String2, PosInt);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.
`PosInt`:   A positive integer. If the distance between `String1` and
            `String2` is greater than `PosInt`, `min_edit_dist()` will stop its
            computation at `PosInt` and return `PosInt`. Make `PosInt` as
            small as you can to improve speed and efficiency. For example,
            if you put `where min_edit_dist(...) <= k` in a `where`-clause, make
            `PosInt` be `k`.

Returns: Either an integer equal to the Levenshtein edit distance (without transpositions)
between `String1` and `String2`, if that distance is minimal among all distances computed
in the query, or some unspecified number greater than the minimum distance computed
in the query.

Example Usage:

    select Name, min_edit_dist(Name, "Vladimir Iosifovich Levenshtein", 6) as EditDist
         from Customers
         order by EditDist, Name asc;

The above will return all rows `(Name, EditDist)` from the `Customers` table.
The rows will be sorted in ascending order by `EditDist` and then by `Name`,
and the first row(s) will have `EditDist` equal to the edit distance (without transpositions)
between `Name` and "Vladimir Iosifovich Levenshtein" or 6, whichever is smaller. All
other rows will have `EditDist`


## Minimum Damarau-Levenshtein Edit Distance / "Closest Match":  `min_edit_dist_t(String1, String2, PosInt)`

Computes the Damarau-Levenshtein edit distance (transpositions) between two strings unless
that distance will exceed `current_min_distance`, the minimum edit distance
it has computed in the query so far, in which case it will return
`current_min_distance + 1`. The `current_min_distance` is initialized to
`PosInt`.

In the common case that we wish to find the rows that have the *smallest*
distance between strings, we can achieve significant performance
improvements if we stop the computation when we know the distance will be
*greater* than some other distance we have already computed.

Syntax:

    min_edit_dist_t(String1, String2, PosInt);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.
`PosInt`:   A positive integer. If the distance between `String1` and
            `String2` is greater than `PosInt`, `min_edit_dist_t()` will stop its
            computation at `PosInt` and return `PosInt`. Make `PosInt` as
            small as you can to improve speed and efficiency. For example,
            if you put `where min_edit_dist_t(...) <= k` in a `where`-clause, make
            `PosInt` be `k`.

Returns: Either an integer equal to the Damarau-Levenshtein edit distance between
`String1` and `String2`, if that distance is minimal among all distances computed
in the query, or some unspecified number greater than the minimum distance computed
in the query.

Example Usage:

    select Name, min_edit_dist_t(Name, "Vladimir Iosifovich Levenshtein", 6) as EditDist
         from Customers
         order by EditDist, Name asc;

The above will return all rows `(Name, EditDist)` from the `Customers` table.
The rows will be sorted in ascending order by `EditDist` and then by `Name`,
and the first row(s) will have `EditDist` equal to the edit distance between
`Name` and "Vladimir Iosifovich Levenshtein" or 6, whichever is smaller. All
other rows will have `EditDist` equal to some other unspecified larger number.


## Minimum Damarau-Levenshtein Similarity / "Closest Match": `min_similarity_t(String1, String2, RealNumber)`

Computes a similarity score in the range [0.0, 1.0] of two strings
unless that similarity score will be less than `current_max_similarity`,
the largest similarity score it has computed in the query so far, in
which case it will return some smaller value. The `current_max_similarity`
is initialized to `RealNumber`.

In the common case that we wish to find the rows that have the *greatest*
similarity between strings, we can achieve *significant* performance
improvements if we stop the computation when we know the similarity will be
*smaller* than some other similarity we have already computed. Under reasonable
conditions, the speed of this function (excluding overhead) can be only twice
as slow as doing nothing at all!

Similarity is computed from Damarau-Levenshtein edit distance by "normalizing"
using the length of the longest string:

    similarity = 1.0 - EditDistance(String1, String2)/max(len(String1), len(String2))

Syntax:

    min_similarity_t(String1, String2, RealNumber);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.
`PosInt`:   A positive integer. If the similarity between `String1` and
            `String2` is less than `RealNumber`, `min_similarity_t()` will stop its
            computation at `RealNumber` and return a number smaller than
            `RealNumber`. Make `RealNumber` as large as you can to improve
            speed and efficiency. For example, if you put
            `where min_similarity_t(...) >= p` in a `where`-clause, make
            `RealNumber` be `p`.

Returns: Either a real number equal to the similarity between `String1` and `String2`,
if that distance is minimal among all distances computed in the query, or some
unspecified number smaller than the minimum distance computed in the query.

Example Usage:

    select Name, min_similarity_t(Name, "Vladimir Iosifovich Levenshtein", 0.75) as Similarity
         from Customers
         order by Similarity, Name desc;

The above will return all rows `(Name, Similarity)` from the `Customers` table.
The rows will be sorted in descending order by `Similarity` and then by `Name`,
and the first row(s) will have `Similarity` equal to the similarity between
`Name` and "Vladimir Iosifovich Levenshtein" or 0.75, whichever is larger. All
other rows will have `EditDist` equal to some other unspecified smaller number.
