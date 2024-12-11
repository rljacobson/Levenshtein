
def print_matrix(s1, s2, dp, arrows):
    # Column headers
    print('       ', end='')
    for i in range(len(s2)):
        print(f"{s2[i]}   ", end='')
    print("\n  ", end='')

    # First row is special
    for i in range(len(s2)+1):
        print(f"{dp[0][i]:>2}", end='')
        if i < len(s2):
            print(' →', end='')


    for i in range(1, len(s1)+1):
        print("\n   ", end='')

        # Print row between cells
        for j in range(0, len(s2)+1):
            if '↓' in arrows[i][j]:
                print('↓ ', end='')
            else:
                print('  ', end='')
            if j < len(s2) and '↘' in arrows[i][j+1]:
                print('↘ ', end='')
            elif j < len(s2) and '⇘' in arrows[i][j+1]:
                print('⇘ ', end='')
            else:
                print('  ', end='')
        print(f"\n{s1[i-1]} ", end='')

        # Print the cells
        for j in range(0, len(s2)+1):
            print(f"{dp[i][j]:>2}", end='')
            if j < len(s2) and '→' in arrows[i][j+1]:
                print(' →', end='')
            else:
                print('  ', end='')
    print()


def levenshtein_distance(s1, s2):
    # Initialize the matrix
    m, n = len(s1), len(s2)
    dp = [[0] * (n + 1) for _ in range(m + 1)]
    arrows = [[[]] * (n + 1) for _ in range(m + 1)]

    # Fill the first row and column
    for i in range(m + 1):
        dp[i][0] = i
        arrows[i][0] = ['↓'] # Deletion
    for j in range(n + 1):
        dp[0][j] = j
        arrows[0][j] = ['→']  # Insertion

    # Compute the distance
    for i in range(1, m + 1):
        for j in range(1, n + 1):
            cost = 0 if s1[i - 1] == s2[j - 1] else 1
            dp[i][j] = min(dp[i - 1][j] + 1,      # Deletion
                           dp[i][j - 1] + 1,      # Insertion
                           dp[i - 1][j - 1] + cost)  # Substitution
            # Determine the arrow direction
            cell_arrows = []
            if cost == 0:
                cell_arrows.append('⇘')  # Characters are equal
            if dp[i][j] == dp[i - 1][j] + 1:
                cell_arrows.append('↓')  # Deletion
            if dp[i][j] == dp[i][j - 1] + 1:
                cell_arrows.append('→')  # Insertion
                # print(f"Insertion: {s1[i-1]} -> {s2[j-1]}")
            if dp[i][j] == dp[i - 1][j - 1] + 1:
                cell_arrows.append('↘')  # Substitution

            arrows[i][j] = cell_arrows

    return dp, arrows

def levbanded(s1, s2, max_cost):
    # Initialize the matrix
    n, m = len(s1), len(s2)
    dp = [[0] * (m + 1) for _ in range(n + 1)]
    arrows = [[[]] * (m + 1) for _ in range(n + 1)]

    # Fill the first row and column
    for i in range(n + 1):
        dp[i][0] = i
        arrows[i][0] = ['↓'] # Deletion
    for j in range(m + 1):
        dp[0][j] = j
        arrows[0][j] = ['→']  # Insertion

    # Compute the distance
    for i in range(1, n + 1):
        min_in_row = max(i, m-n)

        start_j = max(1, i - max_cost)
        end_j = min(m, i + max_cost)

        if start_j > 1:
            dp[i][start_j-1] = max_cost + 1
        if end_j < m:
            dp[i][end_j+1] = max_cost + 1


        for j in range(start_j, end_j + 1):
            cost = 0 if s1[i - 1] == s2[j - 1] else 1
            dp[i][j] = min(dp[i - 1][j] + 1,      # Deletion
                           dp[i][j - 1] + 1,      # Insertion
                           dp[i - 1][j - 1] + cost)  # Substitution

            min_in_row = min(min_in_row, dp[i][j])

            # Determine the arrow direction
            cell_arrows = []
            if cost == 0:
                cell_arrows.append('⇘')  # Characters are equal
            if dp[i][j] == dp[i - 1][j] + 1:
                cell_arrows.append('↓')  # Deletion
            if dp[i][j] == dp[i][j - 1] + 1:
                cell_arrows.append('→')  # Insertion
                # print(f"Insertion: {s1[i-1]} -> {s2[j-1]}")
            if dp[i][j] == dp[i - 1][j - 1] + 1:
                cell_arrows.append('↘')  # Substitution

            arrows[i][j] = cell_arrows

        if min_in_row > max_cost:
            dp[-1][-1] = max_cost + 1
            return dp, arrows

    return dp, arrows

def levbandedopt(s1, s2, max_cost):
    # Initialize the matrix
    n, m = len(s1), len(s2)
    dp = [[0] * (m + 1) for _ in range(n + 1)]
    arrows = [[[]] * (m + 1) for _ in range(n + 1)]

    # Fill the first row and column
    for i in range(n + 1):
        dp[i][0] = i
        arrows[i][0] = ['↓'] # Deletion
    for j in range(m + 1):
        dp[0][j] = j
        arrows[0][j] = ['→']  # Insertion

    # initialize start and end
    start_j = 1
    end_j = min(int((max_cost + m-n)/2 + 1), m)

    # Compute the distance
    for i in range(1, n + 1):

        if start_j > 1:
            dp[i][start_j-1] = max_cost + 1
        if end_j < m:
            dp[i][end_j+1] = max_cost + 1

        min_in_row = max(i, m-n)

        for j in range(start_j, end_j + 1):
            cost = 0 if s1[i - 1] == s2[j - 1] else 1
            dp[i][j] = min(dp[i - 1][j] + 1,      # Deletion
                           dp[i][j - 1] + 1,      # Insertion
                           dp[i - 1][j - 1] + cost)  # Substitution

            min_in_row = min(min_in_row, dp[i][j])

            # Determine the arrow direction
            cell_arrows = []
            if cost == 0:
                cell_arrows.append('⇘')  # Characters are equal
            if dp[i][j] == dp[i - 1][j] + 1:
                cell_arrows.append('↓')  # Deletion
            if dp[i][j] == dp[i][j - 1] + 1:
                cell_arrows.append('→')  # Insertion
                # print(f"Insertion: {s1[i-1]} -> {s2[j-1]}")
            if dp[i][j] == dp[i - 1][j - 1] + 1:
                cell_arrows.append('↘')  # Substitution

            arrows[i][j] = cell_arrows

        # Update start and end
        while end_j > 0 and \
                dp[i][end_j] + abs(end_j - i - m + n) > max_cost:
            end_j = end_j - 1;

        end_j = min(m, end_j + 1)

        while start_j <= end_j and \
                abs(i + m - n - start_j) + dp[i][start_j] > max_cost:
            start_j = start_j + 1

        if end_j < start_j:
            dp[-1][-1] = max_cost + 1
            return dp, arrows

    return dp, arrows


if __name__ == "__main__":
    s1 = "lysmata amboinensis"
    s2 = "lysmmata amhbonensis" # Always make this the longer string
    max_cost = 4

    dp, arrows = levenshtein_distance(s1, s2)
    print(f"\nStandard Algorithm (max_cost = {max_cost})")
    print_matrix(s1, s2,dp, arrows)

    dp, arrows = levbanded(s1, s2, max_cost)
    print(f"\nBanded Algorithm (max_cost = {max_cost})")
    print_matrix(s1, s2,dp, arrows)

    dp, arrows = levbandedopt(s1, s2, max_cost)
    print(f"\nBanded Algorithm Optimized (max_cost = {max_cost})")
    print_matrix(s1, s2,dp, arrows)

    print()
