# two-branch fixture — expected values

| Branch  | HEAD SHA1                                |
|---------|------------------------------------------|
| master  | 2e9e71c9339c4bb19ee155a3012ccae7d8bb4d76 |
| feature | ae86e6faf29fb098a86a9dced7571acaee3e3bd8 |

Shared parent commit (master HEAD = feature's parent): 2e9e71c9339c4bb19ee155a3012ccae7d8bb4d76

## Commit graph

```
master: [Initial commit] <- HEAD of master, parent of feature
feature: [Add feature]   <- HEAD of feature (parent = master HEAD)
```
