# Anno Resource Files (.rda)

## format

two currently known versions

| Version | Used in         |
| ------- | --------------- |
| 2.0     | 1404, 2070      |
| 2.2     | 117, 1800, 2205 |

the file structure is as follow

| Section    | Fields        |
| ---------- | ------------- |
| rda header | magic         |
|            | reserved      |
|            | block pointer |
| rda data   | block data    |
|            | block data    |
|            | block data    |
|            | ...           |
|            | block header  |
|            | block header  |
|            | block header  |
|            | ...           |

the `magic` field describes the archive version.

the `block pointer` locates the first block header and works as entrypoint
to a linked list of block headers to access the block data.

### rda header 2.0

| Field         | Version | Offset | Size |
| ------------- | ------- | ------ | ---- |
| magic         | 2.0     | 0      | 36   |
|               | 2.2     | 0      | 18   |
| reserved      | 2.0     | 36     | 1008 |
|               | 2.2     | 18     | 766  |
| block pointer | 2.0     | 1008   | 4    |
|               | 2.2     | 784    | 8    |

#### magic

* UTF-16 representation of the string `Resource File V2.0`
* UTF-8 representation of the string `Resource File V2.2`

### block header

| Field             | Version | Offset | Size |
| ----------------- | ------- | ------ | ---- |
| flags             |         | 0      | 4    |
| files             |         | 4      | 4    |
| size compressed   | 2.0     | 8      | 4    |
|                   | 2.2     | 8      | 8    |
| size uncompressed | 2.0     | 12     | 4    |
|                   | 2.2     | 16     | 8    |
| block pointer     | 2.0     | 16     | 4    |
|                   | 2.2     | 24     | 8    |

#### flags

| Flag     | Description     |
| -------- | --------------- |
| `0x0001` | compressed      |
| `0x0010` | encrypted       |
| `0x0100` | memory-resident |
| `0x1000` | deleted         |

#### block pointer

the end of the linked list is indicated by a value of `0`.
