const { ipcRenderer } = require('electron');
const WebSocket = require('ws');

// Global variables
let chatSocket = null;
let currentSection = 'wallet';

// Initialize the app
document.addEventListener('DOMContentLoaded', () => {
    initializeWallet();
    connectToChat();
});

// Wallet initialization
async function initializeWallet() {
    try {
        // Get wallet balance
        const balance = await ipcRenderer.invoke('get-balance');
        document.getElementById('balance').textContent = balance;

        // Get wallet address
        const address = await ipcRenderer.invoke('get-address');
        document.getElementById('wallet-address').textContent = address;
        document.getElementById('receive-address').value = address;
        document.getElementById('mining-address').value = address;

        console.log('Wallet initialized successfully');
    } catch (error) {
        console.error('Failed to initialize wallet:', error);
    }
}

// Section navigation
function showSection(sectionName) {
    // Hide all sections
    document.querySelectorAll('.section').forEach(section => {
        section.classList.add('hidden');
    });

    // Remove active class from all nav items
    document.querySelectorAll('.nav-item').forEach(item => {
        item.classList.remove('active');
    });

    // Show selected section
    document.getElementById(sectionName + '-section').classList.remove('hidden');

    // Add active class to clicked nav item
    event.target.classList.add('active');

    currentSection = sectionName;
}

// Send transaction
async function sendTransaction() {
    const address = document.getElementById('send-address').value;
    const amount = document.getElementById('send-amount').value;
    const fee = document.getElementById('send-fee').value;

    if (!address || !amount) {
        alert('Please fill in all required fields');
        return;
    }

    // Validate address format (starts with '1')
    if (!address.startsWith('1')) {
        alert('Invalid Coral address format. Addresses must start with "1"');
        return;
    }

    try {
        const result = await ipcRenderer.invoke('send-transaction', address, amount);

        if (result.success) {
            alert('Transaction sent successfully!');
            // Clear form
            document.getElementById('send-address').value = '';
            document.getElementById('send-amount').value = '';
            document.getElementById('send-fee').value = '0.0001';

            // Refresh balance
            initializeWallet();
        } else {
            alert('Transaction failed: ' + result.error);
        }
    } catch (error) {
        alert('Transaction error: ' + error.message);
    }
}

// Generate new address
async function generateNewAddress() {
    try {
        const newAddress = await ipcRenderer.invoke('get-address');
        document.getElementById('receive-address').value = newAddress;
        alert('New address generated!');
    } catch (error) {
        alert('Failed to generate new address: ' + error.message);
    }
}

// Chat functionality
function connectToChat() {
    try {
        chatSocket = new WebSocket('ws://localhost:8765');

        chatSocket.on('open', () => {
            console.log('Connected to chat server');
            addChatMessage('System', 'Connected to Coral network chat', 'system');
        });

        chatSocket.on('message', (data) => {
            try {
                const message = JSON.parse(data);
                addChatMessage(message.user || 'Anonymous', message.text, 'user');
            } catch (e) {
                // Handle plain text messages
                addChatMessage('Network', data, 'network');
            }
        });

        chatSocket.on('close', () => {
            console.log('Disconnected from chat server');
            addChatMessage('System', 'Disconnected from chat', 'system');
        });

        chatSocket.on('error', (error) => {
            console.error('Chat connection error:', error);
            addChatMessage('System', 'Chat connection error', 'error');
        });
    } catch (error) {
        console.error('Failed to connect to chat:', error);
    }
}

function addChatMessage(user, text, type = 'user') {
    const messagesContainer = document.getElementById('chat-messages');
    const messageElement = document.createElement('div');
    messageElement.className = 'message';

    const timestamp = new Date().toLocaleTimeString();
    messageElement.innerHTML = `<strong>${user}</strong> <span style="opacity: 0.6; font-size: 12px;">${timestamp}</span><br>${text}`;

    // Style based on message type
    switch (type) {
        case 'system':
            messageElement.style.backgroundColor = 'rgba(76, 175, 80, 0.2)';
            break;
        case 'error':
            messageElement.style.backgroundColor = 'rgba(244, 67, 54, 0.2)';
            break;
        case 'network':
            messageElement.style.backgroundColor = 'rgba(33, 150, 243, 0.2)';
            break;
        default:
            messageElement.style.backgroundColor = 'rgba(255, 255, 255, 0.1)';
    }

    messagesContainer.appendChild(messageElement);
    messagesContainer.scrollTop = messagesContainer.scrollHeight;
}

function sendChatMessage() {
    const input = document.getElementById('chat-input');
    const message = input.value.trim();

    if (message && chatSocket && chatSocket.readyState === WebSocket.OPEN) {
        const chatMessage = {
            user: 'You',
            text: message,
            timestamp: Date.now()
        };

        chatSocket.send(JSON.stringify(chatMessage));
        input.value = '';
    } else if (!chatSocket || chatSocket.readyState !== WebSocket.OPEN) {
        addChatMessage('System', 'Not connected to chat server', 'error');
    }
}

function handleChatKeyPress(event) {
    if (event.key === 'Enter') {
        sendChatMessage();
    }
}

// Mining functionality
function startMining() {
    const address = document.getElementById('mining-address').value;

    if (!address) {
        alert('Please set a mining address first');
        return;
    }

    // This would connect to the Coral daemon for mining
    alert('Mining functionality will be implemented when Coral daemon is running.\n\nFeatures:\n- RandomX algorithm\n- ASIC resistant\n- CPU/GPU mining\n- Dynamic difficulty\n- Block reward halving');
}

// Utility functions
function formatCoralAmount(amount) {
    return parseFloat(amount).toFixed(8) + ' CORAL';
}

function isValidCoralAddress(address) {
    return address && address.startsWith('1') && address.length >= 26 && address.length <= 35;
}

// Auto-refresh balance every 30 seconds
setInterval(() => {
    if (currentSection === 'wallet') {
        initializeWallet();
    }
}, 30000);

// Add some animation to the floating shapes
document.addEventListener('DOMContentLoaded', () => {
    const shapes = document.querySelectorAll('.floating-shape');
    shapes.forEach((shape, index) => {
        shape.style.animationDelay = `-${index * 2}s`;
        shape.style.animationDuration = `${6 + index}s`;
    });
});

console.log('🪸 Coral Wallet loaded successfully!');
console.log('Evolution beyond Bitcoin - RandomX - Dynamic blocks - Chat enabled');