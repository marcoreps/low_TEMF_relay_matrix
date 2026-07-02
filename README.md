# low_TEMF_relay_matrix

A low-thermal EMF relay matrix board designed for **automated 4-wire resistance measurements** and **NBS 430 voltage supervision**. Needs 5V supply and an external microcontroller to feed data into a shift register bucket brigade. Oblong terminal pads for cable shoes or brinding posts. <19" wide board for possible rack installation.

---

## Changelog

### Version 0.5
* Exposed GND copper plane around a mounting hole for convenient shield/enclosure chassis connection
* Criss-cross pattern on layers 2,3 instead of identical traces to minimize path resistance

### Version 0.4
* Symmetric annotations
* Added test points connected directly to the four buses
* Added silkscreen logos

---

## Recommended PCB

* 4 layer
* OSP (a bit difficult to strip but gives bare copper pads)
* Gold plating shouldn't be much worse

---

## Images

<div align="center">

### Overview
| 3D Render | Concept |
| :---: | :---: |
| <a href="img/3d_viewer.png"><img src="img/3d_viewer.png" width="300"></a> | <a href="img/concept_documentation.png"><img src="img/concept_documentation.png" width="300"></a> |

### Schematics
| Relay Driver | Relay Block |
| :---: | :---: |
| <a href="img/drivers.png"><img src="img/drivers.png" width="200"></a> | <a href="img/relay_block.png"><img src="img/relay_block.png" width="200"></a> |

### Performance
| Test Results of 0.4|
| :---: |
| <a href="img/benchmark.png"><img src="img/benchmark.png" width="200"></a> |

### Heat spreader example
| Front | Rear | Sandwitch |
| :---: | :---: | :---: |
| <a href="img/PXL_20260630_173706793.jpg"><img src="img/PXL_20260630_173706793.jpg" width="200"></a> | <a href="img/PXL_20260630_174823579.jpg"><img src="img/PXL_20260630_174823579.jpg" width="200"></a> | <a href="img/PXL_20260630_174914501.jpg"><img src="img/PXL_20260630_174914501.jpg" width="200"></a> |

| Taobao Terminals | 3mm Al plate | 19" width |
| :---: | :---: | :---: |
| <a href="img/PXL_20260630_174748971.jpg"><img src="img/PXL_20260630_174748971.jpg" width="200"></a> | <a href="img/PXL_20260630_174942511.jpg"><img src="img/PXL_20260630_174942511.jpg" width="200"></a> | <a href="img/PXL_20260702_090840871.jpg"><img src="img/PXL_20260702_090840871.jpg" width="200"></a> |

</div>

---

