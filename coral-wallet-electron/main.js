const { app, BrowserWindow, ipcMain, Menu } = require('electron');
const path = require('path');
const WebSocket = require('ws');

// Keep reference to window object
let mainWindow;
let chatServer;

function createWindow() {
  // Create the browser window with Microsoft-style aesthetics
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      webSecurity: false
    },
    icon: path.join(__dirname, 'assets', 'coral-icon.png'),
    titleBarStyle: 'default',
    frame: true,
    resizable: true,
    minimizable: true,
    maximizable: true,
    show: false // Don't show until ready
  });

  // Load the app
  mainWindow.loadFile('index.html');

  // Show window when ready (Microsoft-style smooth opening)
  mainWindow.once('ready-to-show', () => {
    mainWindow.show();
    // Focus for that authentic Windows feel
    mainWindow.focus();
  });

  // Handle window closed
  mainWindow.on('closed', () => {
    mainWindow = null;
  });

  // Create chat WebSocket server
  startChatServer();
}

function startChatServer() {
  chatServer = new WebSocket.Server({ port: 8765 });

  chatServer.on('connection', (ws) => {
    console.log('Chat client connected');

    ws.on('message', (message) => {
      // Broadcast to all connected clients
      chatServer.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
          client.send(message);
        }
      });
    });

    ws.on('close', () => {
      console.log('Chat client disconnected');
    });
  });

  console.log('Chat server started on port 8765');
}

// App event handlers
app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});

// IPC handlers for wallet operations
ipcMain.handle('get-balance', async () => {
  // TODO: Connect to Coral daemon
  return '0.00000000 CORAL';
});

ipcMain.handle('send-transaction', async (event, address, amount) => {
  // TODO: Implement transaction sending
  return { success: false, error: 'Not implemented yet' };
});

ipcMain.handle('get-address', async () => {
  // Generate new address (starts with '1' as specified)
  return '1CoralWallet' + Math.random().toString(36).substr(2, 25);
});

// Create minimal menu (very Microsoft-like)
const createMenu = () => {
  const template = [
    {
      label: 'File',
      submenu: [
        { label: 'New Wallet', click: () => {} },
        { label: 'Open Wallet', click: () => {} },
        { type: 'separator' },
        { label: 'Exit', click: () => app.quit() }
      ]
    },
    {
      label: 'View',
      submenu: [
        { role: 'reload' },
        { role: 'forcereload' },
        { role: 'toggledevtools' },
        { type: 'separator' },
        { role: 'resetzoom' },
        { role: 'zoomin' },
        { role: 'zoomout' }
      ]
    },
    {
      label: 'Help',
      submenu: [
        { label: 'About Coral', click: () => {} }
      ]
    }
  ];

  Menu.setApplicationMenu(Menu.buildFromTemplate(template));
};

app.whenReady().then(createMenu);