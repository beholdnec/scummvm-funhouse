"use strict";

const electron = require('electron')
const remote = electron.remote
const dialog = remote.dialog
const fs = require('fs')

const audioCtx = new window.AudioContext()
let bltAudioBuffer = null
let bltAudioSource = null

function makeRgba(r, g, b, a) {
  return (r << 24) | (g << 16) | (b << 8) | a
}

let DEFAULT_PALETTE = new Array(256)
for (let r = 0; r < 4; r++) {
  for (let g = 0; g < 8; g++) {
    for (let b = 0; b < 4; b++) {
      const i = 8 * 4 * r + 4 * g + b
      DEFAULT_PALETTE[i] = makeRgba(Math.trunc(r * 255 / 3),
        Math.trunc(g * 255 / 7), Math.trunc(b * 255 / 3), 255)
    }
  }
}
let currentPalette = DEFAULT_PALETTE

function u8hex(x) {
  const str = x.toString(16).toUpperCase()
  return '0'.repeat(2 - str.length) + str
}

function u16hex(x) {
  const str = x.toString(16).toUpperCase()
  return '0'.repeat(4 - str.length) + str
}

function u32hex(x) {
  const str = x.toString(16).toUpperCase()
  return '0'.repeat(8 - str.length) + str
}

function closeAllResources() {
  if (bltAudioSource) {
    bltAudioSource.stop()
    bltAudioSource = null
    bltAudioBuffer = null
  }

  const contents = document.querySelectorAll('.content-section.is-shown')
  for (let content of contents) {
    content.classList.remove('is-shown')
  }
}

let bltFile = null
let dirTable = null

function myOpen(path) {
  return { file: fs.openSync(path, 'r'), position: 0 }
}

function myRead(file, numBytes) {
  let buf = new Uint8Array(numBytes)
  let numBytesRead = fs.readSync(file.file, buf, 0, numBytes, file.position)
  file.position += numBytesRead
  return buf
}

function myReadU8(file) {
  let buf = myRead(file, 1)
  return new DataView(buf.buffer).getUint8(0)
}

function myReadU16(file) {
  let buf = myRead(file, 2)
  // All BLT fields are in big-endian format.
  return new DataView(buf.buffer).getUint16(0)
}

function myReadU32(file) {
  let buf = myRead(file, 4)
  // All BLT fields are in big-endian format.
  return new DataView(buf.buffer).getUint32(0)
}

function mySkip(file, numBytes) {
  file.position += numBytes
}

function mySeek(file, position) {
  file.position = position
}

function myTell(file) {
  return file.position
}

function convertIndexedToRgba(dstImageData, srcBytes, palette) {
  const dataView = new DataView(dstImageData.data.buffer)
  for (let i = 0; i < srcBytes.length; i++) {
    dataView.setUint32(i * 4, palette[srcBytes[i]])
  }
}

function decodeRL7(dst, src, width, height) {
  let inCursor = 0
  let outX = 0
  let outY = 0
  while (outY < height) {
    if (inCursor >= src.length) {
      break
    }
    const inByte = src[inCursor]
    inCursor++
    const color = inByte & 0x7F
    if (inByte & 0x80) {
      // Run of pixels
      if (inCursor >= src.length) {
        break
      }
      const lengthByte = src[inCursor]
      inCursor++

      // Every line must end with a run of 0 pixels, indicating that the color
      // is drawn til the end of the line and the cursor moves to the next line.
      // This code is necessary to move to the next line even if no pixels are
      // drawn.
      const length = (lengthByte == 0) ? (width - outX) : lengthByte
      if (length != 0) {
        const i = outY * width + outX
        dst.fill(color, i, i + length)
        outX += length
      }

      if (lengthByte == 0) {
        outY++
        outX = 0
      }
    } else {
      // One pixel
      dst[outY * width + outX] = color
      outX++
    }
  }
}

function decompressBoltLZ(dst, src) {
  let inCursor = 0
  let outCursor = 0
  let remaining = dst.length
  while (remaining > 0) {
    const control = src[inCursor]
    inCursor++
    const comp = control >> 6
    const flag = (control >> 5) & 1
    const num = control & 0x1F
    if (comp == 0) {
      // Raw bytes
      const count = 31 - num
      remaining -= count
      dst.set(src.subarray(inCursor, inCursor + count), outCursor)
      outCursor += count
      inCursor += count
    } else if (comp == 1) {
      // Small repeat from previously decoded bytes
      const count = 35 - num
      remaining -= count
      const offset = src[inCursor] + (flag ? 256 : 0)
      inCursor++
      // Bytes must be copied individually for correctness
      for (let i = 0; i < count; i++) {
        dst[outCursor] = dst[outCursor - offset]
        outCursor++
      }
    } else if (comp == 2) {
      // Big repeat from previously decoded bytes
      const count = (32 - num) * 4 + (flag ? 2 : 0)
      remaining -= count
      const offset = src[inCursor] * 2
      inCursor++
      // Bytes must be copied individually for correctness
      for (let i = 0; i < count; i++) {
        dst[outCursor] = dst[outCursor - offset]
        outCursor++
      }
    } else if (comp == 3 && flag) {
      // Original checked for end of data here. We check on every iteration.
      // Sometimes, this check is triggered even though data has not ended.
    } else if (comp == 3 && !flag) {
      // Big block filled with constant byte
      const count = (32 - num + 32 * src[inCursor]) * 4
      inCursor++
      remaining -= count
      inCursor++ // Byte ignored!
      const b = src[inCursor]
      inCursor++
      dst.fill(b, outCursor, outCursor + count)
      outCursor += count
    } else {
      throw new Error("Unreachable")
    }
  }
}

function setDataFields(content, dataFields) {
  for (const [name, val] of Object.entries(dataFields)) {
    content.querySelector(`[data-field="${name}"]`).innerText = val
  }
}

function escapeHtml(str) {
  // From <http://shebang.brandonmintern.com/foolproof-html-escaping-in-javascript/>.
  // Blame him if this is bad!
  const div = document.createElement('div')
  div.appendChild(document.createTextNode(str))
  return div.innerHTML
}

function getField(dataView, field) {
  let val = undefined
  switch (field.type) {
    case 'u8':
      val = dataView.getUint8(field.offset)
      break
    case 'i8':
      val = dataView.getInt8(field.offset)
      break
    case 'x8':
      val = u8hex(dataView.getUint8(field.offset))
      break
    case 'u16':
      val = dataView.getUint16(field.offset)
      break
    case 'i16':
      val = dataView.getInt16(field.offset)
      break
    case 'x16':
      val = u16hex(dataView.getUint16(field.offset))
      break
    case 'u32':
      val = dataView.getUint32(field.offset)
      break
    case 'i32':
      val = dataView.getInt32(field.offset)
      break
    case 'x32':
      val = u32hex(dataView.getUint32(field.offset))
      break
    case 'i16-pair':
      val = `(${dataView.getInt16(field.offset)}, ${dataView.getInt16(field.offset + 2)})`
      break
    case 'rect':
      val = `(${dataView.getInt16(field.offset)}, ${dataView.getInt16(field.offset + 4)}), (${dataView.getInt16(field.offset + 2)}, ${dataView.getInt16(field.offset + 6)})`
      break
    case 'short-res-id':
      // TODO: emit clickable link
      val = `${u16hex(dataView.getUint16(field.offset))}`
      break
    case 'long-res-id':
      // TODO: emit clickable link
      val = `${u32hex(dataView.getUint32(field.offset))}`
      break
    case 'short-reslist-id': // Quantity and short resource ID
      // TODO: emit clickable link
      val = `${u16hex(dataView.getUint16(field.offset + 2))} x ${dataView.getUint16(field.offset)}`
      break
    case 'long-reslist-id': // Quantity and long resource ID
      // TODO: emit clickable link
      val = `${u32hex(dataView.getUint32(field.offset + 2))} x ${dataView.getUint16(field.offset)}`
      break
    case 'custom':
      val = field.custom
      break
    default:
      throw new Error(`Invalid field type '${field.type}'`)
  }

  return val
}

function emitDataFields(dataView, tableEl, fields) {
  tableEl.innerHTML = ""
  for (const field of fields) {
    const trEl = document.createElement('tr')
    tableEl.appendChild(trEl)
    trEl.innerHTML = `<th>${field.name}</th><td>${escapeHtml(getField(dataView, field))}</td>`
  }
}

function emitDataFieldsArray(dataView, bytesPerItem, tableEl, fields) {
  tableEl.innerHTML = ""

  // Emit table head
  const theadEl = document.createElement('thead')
  tableEl.appendChild(theadEl)
  const trEl = document.createElement('tr')
  theadEl.appendChild(trEl)
  trEl.innerHTML = `<th>No.</th>`
  for (const field of fields) {
    trEl.innerHTML += `<th>${escapeHtml(field.name)}</th>`
  }

  // Emit table body
  const tbodyEl = document.createElement('tbody')
  tableEl.appendChild(tbodyEl)
  const numItems = dataView.byteLength / bytesPerItem
  for (let i = 0; i < numItems; i++) {
    const trEl = document.createElement('tr')
    tbodyEl.appendChild(trEl)
    trEl.innerHTML = `<th>${i}</th>`
    for (const field of fields) {
      const fieldDataView = new DataView(dataView.buffer.slice(i * bytesPerItem, (i + 1) * bytesPerItem))
      trEl.innerHTML += `<td>${escapeHtml(getField(fieldDataView, field))}`
    }
  }
}

function openBltFile(path) {
  bltFile = null
  dirTable = null

  const fileSize = fs.statSync(path).size
  bltFile = myOpen(path)

  const magic = myRead(bltFile, 4)
  if (!Buffer.from(magic).equals(Buffer.from('BOLT'))) {
    throw new Error("Invalid file identifier.")
  }

  // Skip unknown fields.
  mySkip(bltFile, 7)

  const numDirectories = myReadU8(bltFile)

  const fileSizeField = myReadU32(bltFile)
  if (fileSizeField != fileSize) {
    throw new Error("Invalid file size field.")
  }

  dirTable = []
  for (let i = 0; i < numDirectories; i++) {
    let newDir = {
      numResources: myReadU32(bltFile),
      compBufSize: myReadU32(bltFile),
      position: myReadU32(bltFile),
      resourceTable: []
    }
    // Skip unknown field.
    myReadU32(bltFile)

    const cursor = myTell(bltFile)
    mySeek(bltFile, newDir.position)
    for (let j = 0; j < newDir.numResources; j++) {
      const typeField = myReadU32(bltFile)
      const resRecord = {
        type: typeField & 0x00FFFFFF,
        compression: typeField >> 24,
        size: myReadU32(bltFile),
        position: myReadU32(bltFile)
      }
      // Skip unknown field (CRC?).
      myReadU32(bltFile)

      newDir.resourceTable.push(resRecord)
    }
    mySeek(bltFile, cursor)

    dirTable.push(newDir)
  }

  for (let el of document.querySelectorAll('#directories tbody')) {
    el.parentNode.removeChild(el)
  }

  const directoriesEl = document.getElementById('directories')
  for (const [dirNum, dir] of dirTable.entries()) {
    let tbodyEl = document.createElement('tbody')
    directoriesEl.appendChild(tbodyEl)
    tbodyEl.setAttribute('data-dir-num', dirNum)
    tbodyEl.innerHTML = `<tr><th>${u8hex(dirNum)}</th><td></td></tr>`

    tbodyEl = document.createElement('tbody')
    directoriesEl.appendChild(tbodyEl)
    tbodyEl.setAttribute('class', 'nav-dropdown')
    tbodyEl.setAttribute('data-dir-num', dirNum)

    for (const [resNum, res] of dir.resourceTable.entries()) {
      const trEl = document.createElement('tr')
      tbodyEl.appendChild(trEl)
      trEl.setAttribute('data-dir-num', dirNum)
      trEl.setAttribute('data-res-num', resNum)
      trEl.innerHTML = `<td>${u16hex(dirNum << 8 | resNum)}</td><td>${res.type}</td>`
    }
  }

  const isNotLoaded = document.querySelectorAll('.is-not-loaded.is-shown')
  for (let el of isNotLoaded) {
    el.classList.remove('is-shown')
  }

  const isLoaded = document.querySelectorAll('.is-loaded')
  for (let el of isLoaded) {
    el.classList.add('is-shown')
  }
}

function openBltUnsigned8BitValueList(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Unsigned 8-Bit Value List'

  const dataView = new DataView(data.buffer)

  emitDataFieldsArray(dataView, 1, contentEl.querySelector('.fields-table'), [
    { name: 'Value', type: 'u8', offset: 0 },
  ])
}

function openBltSigned16BitValueList(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Signed 16-Bit Value List'

  const dataView = new DataView(data.buffer)

  emitDataFieldsArray(dataView, 2, contentEl.querySelector('.fields-table'), [
    { name: 'Value', type: 'i16', offset: 0 },
  ])
}

function openBltUnsigned16BitValueList(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Unsigned 16-Bit Value List'

  const dataView = new DataView(data.buffer)

  emitDataFieldsArray(dataView, 2, contentEl.querySelector('.fields-table'), [
    { name: 'Value', type: 'u16', offset: 0 },
  ])
}

// Type 6
function openBltResourceList(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Resource List'

  const dataView = new DataView(data.buffer)

  emitDataFieldsArray(dataView, 4, contentEl.querySelector('.fields-table'), [
    { name: 'Value', type: 'long-res-id', offset: 0 },
  ])
}

// Type 7
function openBltSound(data) {
  const contentEl = document.getElementById('blt-sound-content')
  contentEl.classList.add('is-shown')

  bltAudioBuffer = audioCtx.createBuffer(1, data.length, 22050)
  const audioChannelData = bltAudioBuffer.getChannelData(0)
  for (const [i, sample] of data.entries()) {
    audioChannelData[i] = (sample / 255.0) * 2.0 - 1.0
  }
}

// Type 8
function openBltImage(data) {
  const contentEl = document.getElementById('blt-image-content')
  contentEl.classList.add('is-shown')

  const dataView = new DataView(data.buffer)

  const header = {
    compression: data[0],
    offsetX: dataView.getInt16(6),
    offsetY: dataView.getInt16(8),
    width: dataView.getUint16(0xA),
    height: dataView.getUint16(0xC),
  }

  const IMAGE_COMPRESSION_TYPES = {0: 'CLUT7', 1: 'RL7'}
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Compression', type: 'custom',
      custom: IMAGE_COMPRESSION_TYPES[header.compression] || 'Unknown' },
    { name: 'Offset', type: 'i16-pair', offset: 6 },
    { name: 'Size', type: 'i16-pair', offset: 0xA },
  ])

  const canvasEl = document.getElementById('blt-image-canvas')
  canvasEl.width = header.width
  canvasEl.height = header.height
  canvasEl.style.width = `${header.width * 2}px`
  canvasEl.style.height = `${header.height * 2}px`
  let ctx = canvasEl.getContext('2d')
  let imageData = ctx.createImageData(header.width, header.height)
  let imageDataView = new DataView(imageData.data.buffer)
  if (header.compression == 0) {
    // CLUT7
    convertIndexedToRgba(imageData, data.slice(0x18), currentPalette)
  } else if (header.compression == 1) {
    // RL7
    const decodedImage = new Uint8Array(header.width * header.height)
    decodeRL7(decodedImage, data.slice(0x18), header.width, header.height)
    convertIndexedToRgba(imageData, decodedImage, currentPalette)
  }
  ctx.putImageData(imageData, 0, 0)
}

// Type 10
function openBltPalette(data) {
  const contentEl = document.getElementById('blt-palette-content')
  contentEl.classList.add('is-shown')

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Plane', type: 'u16', offset: 0 },
    { name: 'Start Index', type: 'u16', offset: 2 },
    { name: 'End Index', type: 'u16', offset: 4 },
  ])

  const startIndex = dataView.getUint16(2)
  const endIndex = dataView.getUint16(4)
  const palette = data.slice(6)

  currentPalette = new Array(256)

  const tableEl = contentEl.querySelector('.palette-table')
  tableEl.innerHTML = ""
  let i = startIndex
  while (i < endIndex) {
    const trEl = document.createElement('tr')
    tableEl.appendChild(trEl)
    for (let j = 0; j < 16; j++) {
      if (i > endIndex) {
        break;
      }
      const tdEl = document.createElement('td')
      const r = palette[3 * i]
      const g = palette[3 * i + 1]
      const b = palette[3 * i + 2]
      currentPalette[i] = makeRgba(r, g, b, 255)
      tdEl.style.backgroundColor = `rgb(${r},${g},${b})`
      trEl.appendChild(tdEl)
      i++
    }
  }
}

// Type 11
function openBltColorCycles(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Color Cycles'

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Slot 0', type: 'custom', custom: `${u32hex(dataView.getUint32(8))} x ${dataView.getUint16(0)}` },
    { name: 'Slot 1', type: 'custom', custom: `${u32hex(dataView.getUint32(0xC))} x ${dataView.getUint16(2)}` },
    { name: 'Slot 2', type: 'custom', custom: `${u32hex(dataView.getUint32(0x10))} x ${dataView.getUint16(4)}` },
    { name: 'Slot 3', type: 'custom', custom: `${u32hex(dataView.getUint32(0x14))} x ${dataView.getUint16(6)}` },
  ])
}

// Type 12
function openBltColorCycleSlot(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Color Cycle Slot'

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Start Index', type: 'u16', offset: 0 },
    { name: 'End Index', type: 'u16', offset: 2 },
    { name: 'Frames', type: 'u8', offset: 4 },
    { name: 'Plane', type: 'u8', offset: 5 },
  ])
}

// Type 26
function openBltPlane(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Plane'

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Image', type: 'long-res-id', offset: 0 },
    { name: 'Palette', type: 'long-res-id', offset: 4 },
    { name: 'Hotspots', type: 'long-res-id', offset: 8 },
  ])
}

// Type 27
function openBltSprite(data) {
  // FIXME: single sprite or array of sprites?
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Sprite'

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Position', type: 'i16-pair', offset: 0 },
    { name: 'Image', type: 'long-res-id', offset: 4 },
  ])
}

// Type 30
function openBltButtonGraphics(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Button Graphics'

  const dataView = new DataView(data.buffer)
  const BUTTON_GRAPHICS_TYPES = {1: 'Palette Mods', 2: 'Sprites'}
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Type', type: 'custom', custom: `${BUTTON_GRAPHICS_TYPES[dataView.getUint16(0)] || 'Unknown'}` },
    { name: 'Hovered Resource', type: 'long-res-id', offset: 6 },
    { name: 'Idle Resource', type: 'long-res-id', offset: 0xA },
  ])
}

// Type 31
function openBltButtonList(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Button List'

  const dataView = new DataView(data.buffer)
  const BUTTON_TYPES = {1: 'Rectangle', 2: 'Display Query', 3: 'Hotspot Query'}
  emitDataFieldsArray(dataView, 0x14, contentEl.querySelector('.fields-table'), [
    { name: 'Type', type: 'custom', custom: `${BUTTON_TYPES[dataView.getUint16(0)] || 'Unknown'}` },
    { name: 'Hitbox', type: 'rect', offset: 2 },
    { name: 'Plane', type: 'u16', offset: 0xA },
    { name: 'Button Graphics', type: 'long-res-id', offset: 0x10 },
  ])
}

// Type 32
function openBltScene(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Scene'

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Foreground', type: 'long-res-id', offset: 0 },
    { name: 'Background', type: 'long-res-id', offset: 4 },
    { name: 'Sprites', type: 'custom', custom: `${u32hex(dataView.getUint32(0xA))} x ${dataView.getUint8(8)}` },
    { name: 'Color Cycles', type: 'long-res-id', offset: 0x16 },
    { name: 'Buttons', type: 'long-reslist-id', offset: 0x1A },
    { name: 'Origin', type: 'i16-pair', offset: 0x20 },
  ])
}

// Type 33
function openBltMainMenu(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Main Menu'

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Scene', type: 'long-res-id', offset: 0 },
    { name: 'Color Bars Image', type: 'long-res-id', offset: 4 },
    { name: 'Color Bars Palette', type: 'long-res-id', offset: 8 },
  ])
}

// Type 40
function openBltHub(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Hub'

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Scene', type: 'long-res-id', offset: 0 },
    { name: 'Background', type: 'long-res-id', offset: 6 },
    { name: 'Item List', type: 'custom', custom: `${u32hex(dataView.getUint32(0xC))} x ${dataView.getUint8(0xB)}`}
  ])
}

// Type 41
function openBltHubItem(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Hub Item'

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, 0x10, contentEl.querySelector('.fields-table'), [
    { name: 'Image', type: 'long-res-id', offset: 4 },
  ])
}

function openBltSlidingPuzzle(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Sliding Puzzle'

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Difficulty 1', type: 'short-res-id', offset: 2 },
    { name: 'Difficulty 2', type: 'short-res-id', offset: 6 },
    { name: 'Difficulty 3', type: 'short-res-id', offset: 0xA },
  ])
}

function openBltParticleDeaths(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Particle Deaths'

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Type 1 Frames', type: 'long-reslist-id', offset: 0 },
    { name: 'Type 2 Frames', type: 'long-reslist-id', offset: 6 },
    { name: 'Type 3 Frames', type: 'long-reslist-id', offset: 0xC },
  ])
}

function openBltPotionPuzzle(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Potion Puzzle'

  const dataView = new DataView(data.buffer)
  emitDataFields(dataView, contentEl.querySelector('.fields-table'), [
    { name: 'Difficulties', type: 'long-res-id', offset: 0 },
    { name: 'Background Image', type: 'long-res-id', offset: 4 },
    { name: 'Background Palette', type: 'long-res-id', offset: 8 },
    { name: 'Shelf Points', type: 'long-reslist-id', offset: 0x16 },
    { name: 'Basin Points', type: 'long-res-id', offset: 0x20 },
    { name: 'Origin', type: 'i16-pair', offset: 0x42 },
  ])
}

// Type 63
function openBltPotionComboList(data) {
  const contentEl = document.getElementById('blt-generic-content')
  contentEl.classList.add('is-shown')
  contentEl.querySelector('header').innerText = 'Potion Combo List'

  const dataView = new DataView(data.buffer)
  // Potion movies extracted from MERLIN.EXE
  // TODO: Utilize
  const POTION_MOVIES = [
    'ELEC', 'EXPL', 'FLAM', 'FLSH', 'MIST', 'OOZE', 'SHMR',
    'SWRL', 'WIND', 'BOIL', 'BUBL', 'BSPK', 'FBRS', 'FCLD',
    'FFLS', 'FSWR', 'LAVA', 'LFIR', 'LSMK', 'SBLS', 'SCLM',
    'SFLS', 'SPRE', 'WSTM', 'WSWL', 'BUGS', 'CRYS', 'DNCR',
    'FISH', 'GLAC', 'GOLM', 'EYEB', 'MOLE', 'MOTH', 'MUDB',
    'ROCK', 'SHTR', 'SLUG', 'SNAK', 'SPKB', 'SPKM', 'SPDR',
    'SQID', 'CLOD', 'SWIR', 'VOLC', 'WORM',
  ]
  emitDataFieldsArray(dataView, 6, contentEl.querySelector('.fields-table'), [
    { name: 'A', type: 'i8', offset: 0 },
    { name: 'B', type: 'i8', offset: 1 },
    { name: 'C', type: 'i8', offset: 2 },
    { name: 'D', type: 'i8', offset: 3 },
    { name: 'Movie', type: 'u16', offset: 4 },
  ])
}

const PC_RESOURCE_LOADERS = {
  1: openBltUnsigned8BitValueList,
  2: openBltSigned16BitValueList,
  3: openBltUnsigned16BitValueList,
  6: openBltResourceList,
  7: openBltSound,
  8: openBltImage,
  10: openBltPalette,
  11: openBltColorCycles,
  12: openBltColorCycleSlot,
  26: openBltPlane,
  27: openBltSprite,
  30: openBltButtonGraphics,
  31: openBltButtonList,
  32: openBltScene,
  33: openBltMainMenu,
  40: openBltHub,
  41: openBltHubItem,
  44: openBltSlidingPuzzle,
  45: openBltParticleDeaths,
  59: openBltPotionPuzzle,
  63: openBltPotionComboList,
}

function openResource(resourceId) {
  const dirNum = resourceId >> 8
  const resNum = resourceId & 0xFF
  const dirTableEntry = dirTable[dirNum]
  const resTableEntry = dirTableEntry.resourceTable[resNum]

  mySeek(bltFile, resTableEntry.position)
  let data = null
  switch (resTableEntry.compression) {
    case 0:
      // BOLT-LZ
      const compressedData = myRead(bltFile, dirTableEntry.compBufSize)
      data = new Uint8Array(resTableEntry.size)
      decompressBoltLZ(data, compressedData)
      break
    case 8:
      // Raw
      data = myRead(bltFile, resTableEntry.size)
      break
    default:
      throw new Error(`Invalid compression type ${resTableEntry.compression}`)
  }

  for (let el of document.querySelectorAll('.is-highlighted')) {
    el.classList.remove('is-highlighted')
  }
  const highlightMe = document.querySelector(`.nav-dropdown [data-dir-num="${dirNum}"][data-res-num="${resNum}"]`)
  highlightMe.classList.add('is-highlighted')

  closeAllResources()

  const labelEl = document.getElementById('blt-resource-label')
  labelEl.innerText = u16hex(resourceId)

  if (resTableEntry.type in PC_RESOURCE_LOADERS) {
    PC_RESOURCE_LOADERS[resTableEntry.type](data)
  }

  const hexContentEl = document.getElementById('blt-hex-content')
  const hexDataEl = document.getElementById('blt-hex-data')
  hexContentEl.classList.add('is-shown')
  if (resTableEntry.size > 0x1000) {
    for (let el of document.querySelectorAll('.is-data-too-large')) {
      el.classList.add('is-shown')
    }
    hexDataEl.classList.remove('is-shown')
  } else {
    for (let el of document.querySelectorAll('.is-data-too-large.is-shown')) {
      el.classList.remove('is-shown')
    }
    hexDataEl.classList.add('is-shown')
    hexDataEl.innerText = ""
    let i = 0
    let hexStr = ""
    while (i < data.length) {
      for (let j = 0; j < 16; ++j) {
        if (i >= data.length) {
          break;
        }
        hexStr += `${u8hex(data[i])} `
        i++
      }
      hexStr += '\n'
    }
    hexDataEl.innerText = hexStr
  }
}

function onClick(target) {
  if (target.dataset.dirNum && target.dataset.resNum) {
    openResource(target.dataset.dirNum << 8 | target.dataset.resNum)
  } else if (target.dataset.dirNum) {
    const directoriesEl = document.getElementById('directories')
    const navDropdown = directoriesEl.querySelector(`.nav-dropdown[data-dir-num="${target.dataset.dirNum}"]`)
    if (navDropdown.classList.contains('is-shown')) {
      navDropdown.classList.remove('is-shown')
    } else {
      navDropdown.classList.add('is-shown')
    }
  } else if (target.parentElement) {
    onClick(target.parentElement)
  }
}

document.body.addEventListener('click', function(event) {
  onClick(event.target)
})

document.getElementById('open-file').addEventListener('click', function (event) {
  dialog.showOpenDialog({
    filters: [
      {name: 'BLT Files', extensions: ['BLT']},
      {name: 'All Files', extensions: ['*']}
    ],
    properties: ['openFile']
  }, function (files) {
    if (files) {
      openBltFile(files[0])
      openResource(0x9901)
    }
  })
})

for (let el of document.querySelectorAll('.is-not-loaded')) {
  el.classList.add('is-shown')
}
for (let el of document.querySelectorAll('.is-loaded.is-shown')) {
  el.classList.remove('is-shown')
}

document.getElementById('blt-sound-play-btn').addEventListener('click', function (event) {
  if (bltAudioSource) {
    bltAudioSource.stop()
    bltAudioSource = null
  }

  bltAudioSource = audioCtx.createBufferSource()
  bltAudioSource.buffer = bltAudioBuffer
  bltAudioSource.connect(audioCtx.destination)
  bltAudioSource.start()
})
