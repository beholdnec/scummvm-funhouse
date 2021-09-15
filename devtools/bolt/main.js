"use strict";
// Runs in the main process
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
const electron_1 = require("electron");
let mainWindow;
electron_1.ipcMain.handle('open-file', (event) => __awaiter(void 0, void 0, void 0, function* () {
    const files = yield electron_1.dialog.showOpenDialog(null, {
        filters: [
            { name: 'BLT Files', extensions: ['BLT'] },
            { name: 'All Files', extensions: ['*'] }
        ],
        properties: ['openFile']
    });
    if (files) {
        return files.filePaths[0];
    }
    else {
        return undefined;
    }
}));
function createWindow() {
    mainWindow = new electron_1.BrowserWindow({
        width: 1024,
        height: 768,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
        },
    });
    mainWindow.on('closed', function () {
        mainWindow = null;
    });
    mainWindow.loadFile('index.html');
}
electron_1.app.on('ready', createWindow);
