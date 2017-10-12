import {app, BrowserWindow} from 'electron'
import * as path from 'path'
import * as url from 'url'

let mainWindow: BrowserWindow

function createWindow() {
  mainWindow = new BrowserWindow({width: 1024, height: 768})

  mainWindow.loadURL(url.format({
    pathname: path.join(__dirname, 'index.html'),
    protocol: 'file:',
    slashes: true
  }))

  mainWindow.on('closed', function () {
    mainWindow = null
  })
}

app.on('ready', createWindow)
