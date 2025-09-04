# Anno Resource Files (.rda)

## format

two currently known versions

| Version | Used in         |
| ------- | --------------- |
| 2.0     | 1404, 2070      |
| 2.2     | 117, 1800, 2205 |

### header 2.0

| Offset | Size | Field         |
| ------ | ---- | ------------- |
| 0      | 36   | magic         |
| 36     | 1008 | reserved      |
| 1008   | 4    | block pointer |

#### magic

UTF-16 representation of the string `Resource File V2.0`

### header 2.2

| Offset | Size | Field         |
| ------ | ---- | ------------- |
| 0      | 18   | magic         |
| 18     | 766  | reserved      |
| 784    | 8    | block pointer |

#### magic

UTF-8 representation of the string `Resource File V2.2`
