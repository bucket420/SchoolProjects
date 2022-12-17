Phase 1:
- Password used: zenith
- Alternatives: any string that starts with "zenith", such as zenithabc,
  zenith123, etc.

Phase 2:
- Password used: 4321
- Alternatives: any string containing more than 3 characters that are not in
  ascending order, such as 5432, dcba, 955321, etc.

Phase 3:
- Password used: liminal
- Alternatives: any string in `kDictionary` whose index is a greater-than-3 prime number. E.g. omega, alpha, etc.

Phase 4:
- Password used: 1 2 3 4 5 0
- Alternatives: any space-separated sequence of 0, 1, 2, 3, 4, 5 such that
  `sequence[sequence[i]] != i`. E.g. 2 3 1 4 5 0, 3 4 0 5 2 1, etc.

Phase 5:
- Password used: 2 6 7 3
- Alternatives: none. x = 8, y = [6, 2, 5, 1]. Password must be a 
  space-separated sequence of length 4 such that `x - y[i] - sequence[i] = 0`.

Phase 6?
- The function `strange` has the same format as the other phase function but
  didn't get called. It will return true if the input is "lacunae".
