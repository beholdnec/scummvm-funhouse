// Runs in the main process

import {app, BrowserWindow, dialog, ipcMain} from 'electron'

let mainWindow: BrowserWindow

ipcMain.handle('open-file', async (event) => {
  const files = await dialog.showOpenDialog(null, {
    filters: [
      {name: 'BLT Files', extensions: ['BLT']},
      {name: 'All Files', extensions: ['*']}
    ],
    properties: ['openFile']
  })

  if (files) {
    return files.filePaths[0]
  } else {
    return undefined
  }
})

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1024,
    height: 768,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
    },
  })

  mainWindow.on('closed', function () {
    mainWindow = null
  })

  mainWindow.loadFile('index.html')
}

app.on('ready', createWindow)
