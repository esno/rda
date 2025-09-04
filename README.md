# Anno Resource Files (.rda)

Anno is one of the city builder and economy simulator.
this tool shall extract it's game data to allow game modifications
on linux-based systems.

> be aware that it is in a very early stage

## compile from source

    make

## usage

    $ ./rda -xf ui.rda 
    [**] block 0x05545c1a (c=0;e=0;r=0;d=0 19040/19040)
    [--] > data/fonts/31df-udmarugothic-w4.ttc
    [**] block 0x8ab1693a (c=0;e=0;r=0;d=0 5738320/5738320)
    [--] > data/ui/2kimages/4k/base/design_system/backgrounds/bg_badge_squarebase_20x8_0.dds
    [**] block 0xb6045b12 (c=0;e=0;r=0;d=0 1268960/1268960)
    [--] > data/ui/4k/base/_test/_test_0.dds
    [**] block 0xb8bfe11e (c=0;e=0;r=0;d=0 136080/136080)
    [--] > data/ui/4kimages/fp/assets/common/bg_bluefabric_tile_64_0.dds
    [**] block 0xb8d3d3a7 (c=0;e=0;r=0;d=0 6720/6720)
    [--] > data/ui/credits/bmwe_fz_2017_office_farbe_en.png
    [**] block 0xb8d94992 (c=0;e=0;r=0;d=0 45920/45920)
    [--] > data/ui/pwl/components/alignmentutility_b973e271-9d9d-454b-82a2-f28ba5347b29.json
    [**] block 0xc0fc5246 (c=0;e=0;r=0;d=0 2198560/2198560)
    [--] > data/ui/studio/generated/0000641a-9eed-4cce-830f-7046dae1548f
    [**] block 0xc1945df6 (c=0;e=0;r=0;d=0 2240/2240)
    [--] > data/ui/studio/atlases/console_shared_asset_01_0.dds
    [**] block 0xc1bfd1d9 (c=0;e=0;r=0;d=0 1680/1680)
    [--] > data/ui/runtimepng/bgr_minimap_undiscovered.png
    [**] block 0xc1bfd1f9 (c=0;e=0;r=0;d=0 0/0)
    [--] >

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

the end of the linked list is indicated by an empty block with pointer value of `0`.
