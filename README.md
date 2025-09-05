# Anno Resource Files (.rda)

Anno is one of the greatest city builder and economy simulators out there.
this tool shall extract it's game data to allow game modifications
on linux-based systems.

> be aware that it is in a very early stage

## compile from source

    make

## usage

    $ ./rda -xf ui.rda
    [**] block 0x05545c1a (c=0;e=0;r=0;d=0 [n=34] 19040/19040) => 0x8ab1693a
    [--] > data/fonts/31df-udmarugothic-w4.ttc@0x00000318 (m=1692287079 4090762/4090762) => 0x055413ea
    [--] > data/fonts/32df-udmarugothic-w6.ttc@0x003e6ea2 (m=1692287079 4072378/4072378) => 0x0554161a
    [--] > data/fonts/adonis-regular.otf@0x007c925c (m=1718268012 109076/109076) => 0x0554184a
    [--] > data/fonts/albertusnova-regular.otf@0x007e3c70 (m=1724265053 197464/197464) => 0x05541a7a
    [--] > data/fonts/albertusnova_eval250512-regular.otf@0x00813fc8 (m=1747138539 174520/174520) => 0x05541caa
    [--] > data/fonts/albertusnovaforanno117tab.otf@0x0083e980 (m=1747829164 178532/178532) => 0x05541eda

    [...]

    [--] > data/fonts/pelago-regular.otf@0x054b6266 (m=1718268012 283548/283548) => 0x055457ba
    [--] > data/fonts/roboto-light.ttf@0x054fb602 (m=1692287080 140276/140276) => 0x055459ea
    [--] > data/fonts/roboto-regular.ttf@0x0551d9f6 (m=1692287080 145348/145348) => 0x05545c1a
    [**] block 0x8ab1693a (c=0;e=0;r=0;d=0 [n=10247] 5738320/5738320) => 0xb6045b12
    [--] > data/ui/2kimages/4k/base/design_system/backgrounds/bg_badge_squarebase_20x8_0.dds@0x05545c3a (m=1738611313 3620/3620) => 0x8a59dc1a
    [--] > data/ui/2kimages/4k/base/design_system/strokes/stroke4_btn_selected_12_0.dds@0x05546a5e (m=1745582519 3268/3268) => 0x8a59de4a

    [...]

## format

there are two known versions out there

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
| rda data   | file data     |
|            | file data     |
|            | file data     |
|            | ...           |
|            | file header   |
|            | file header   |
|            | file header   |
|            | ...           |
|            | block header  |
|            | file data     |
|            | file data     |
|            | ...           |
|            | file header   |
|            | file header   |
|            | ...           |
|            | block header  |
|            | ...           |

the `magic` field describes the archive version.

the `block pointer` locates the first block header and works as entrypoint
to a linked list of block headers to access the block data.

### rda header

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

the end of the linked list is indicated by an empty block with pointer value of `0`.

### file header

| Field             | Version | Offset | Size |
| ----------------- | ------- | ------ | ---- |
| path              |         | 0      | 520  |
| data pointer      | 2.0     | 520    | 4    |
|                   | 2.2     | 520    | 8    |
| size compressed   | 2.0     | 524    | 4    |
|                   | 2.2     | 528    | 8    |
| size uncompressed | 2.0     | 528    | 4    |
|                   | 2.2     | 536    | 8    |
| mtime             | 2.0     | 532    | 4    |
|                   | 2.2     | 544    | 8    |
| reserved          | 2.0     | 536    | 4    |
|                   | 2.2     | 552    | 8    |

the location of the first `file header` is right before the `block header`.
> file header offset = block header offset - size compressed

since one block can contain many files one has to traverse all `file header`
starting from the first.
> file header offset = block header offset - size compressed + (file header size * n)

### path

the file path and name (e.g. `data/fonts/31df-udmarugothic-w4.ttc`).
it is an array of null-terminated characters (`d\0a\0t\0`...).
