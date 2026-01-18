#!/usr/bin/env python3
"""
Coral Web Dashboard - Simple web interface for Coral node
"""

from flask import Flask, render_template, jsonify, request
import subprocess
import json
import os

app = Flask(__name__)

# Configuration
CORAL_CLI = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'src', 'coral-cli')
# Default data directory: ~/Library/Application Support/Coral on macOS
DATADIR = os.path.expanduser('~/Library/Application Support/Coral')

def coral_cli(command, *args):
    """Execute coral-cli command and return result"""
    cmd = [CORAL_CLI, f'-datadir={DATADIR}'] + command.split() + list(args)
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode == 0:
            try:
                return json.loads(result.stdout)
            except json.JSONDecodeError:
                return result.stdout.strip()
        else:
            return {'error': result.stderr.strip()}
    except subprocess.TimeoutExpired:
        return {'error': 'Command timed out'}
    except Exception as e:
        return {'error': str(e)}

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/info')
def get_info():
    """Get blockchain and network info"""
    blockchain = coral_cli('getblockchaininfo')
    network = coral_cli('getnetworkinfo')
    mining = coral_cli('getmininginfo')
    return jsonify({
        'blockchain': blockchain,
        'network': network,
        'mining': mining
    })

@app.route('/api/wallets')
def get_wallets():
    """List all wallets"""
    wallets = coral_cli('listwallets')
    return jsonify({'wallets': wallets if isinstance(wallets, list) else []})

@app.route('/api/wallet/<name>')
def get_wallet_info(name):
    """Get specific wallet info"""
    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'getwalletinfo']
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode == 0:
            return jsonify(json.loads(result.stdout))
        return jsonify({'error': result.stderr.strip()})
    except Exception as e:
        return jsonify({'error': str(e)})

@app.route('/api/wallet/<name>/balance')
def get_balance(name):
    """Get wallet balance"""
    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'getbalance']
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode == 0:
            return jsonify({'balance': float(result.stdout.strip())})
        return jsonify({'error': result.stderr.strip()})
    except Exception as e:
        return jsonify({'error': str(e)})

@app.route('/api/wallet/<name>/newaddress', methods=['GET', 'POST'])
def new_address(name):
    """Generate new receiving address"""
    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'getnewaddress']
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode == 0:
            return jsonify({'address': result.stdout.strip()})
        return jsonify({'error': result.stderr.strip()})
    except Exception as e:
        return jsonify({'error': str(e)})

@app.route('/api/wallet/<name>/transactions')
def get_transactions(name):
    """Get recent transactions"""
    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'listtransactions', '*', '20']
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode == 0:
            return jsonify({'transactions': json.loads(result.stdout)})
        return jsonify({'error': result.stderr.strip()})
    except Exception as e:
        return jsonify({'error': str(e)})

@app.route('/api/block/<hash_or_height>')
def get_block(hash_or_height):
    """Get block by hash or height"""
    # If numeric, get hash first
    if hash_or_height.isdigit():
        blockhash = coral_cli('getblockhash', hash_or_height)
        if isinstance(blockhash, dict) and 'error' in blockhash:
            return jsonify(blockhash)
    else:
        blockhash = hash_or_height

    block = coral_cli('getblock', blockhash)
    return jsonify(block)

@app.route('/api/peers')
def get_peers():
    """Get connected peers"""
    peers = coral_cli('getpeerinfo')
    return jsonify({'peers': peers if isinstance(peers, list) else []})

@app.route('/api/mempool')
def get_mempool():
    """Get mempool info"""
    info = coral_cli('getmempoolinfo')
    return jsonify(info)

@app.route('/api/createwallet', methods=['POST'])
def create_wallet():
    """Create a new wallet"""
    data = request.json
    name = data.get('name', 'wallet')
    # Create legacy wallet for compatibility
    result = coral_cli('createwallet', name, 'false', 'false', '', 'false', 'false')
    return jsonify(result)

@app.route('/api/mining')
def get_mining_info():
    """Get mining information"""
    info = coral_cli('getmininginfo')
    return jsonify(info)

@app.route('/api/mining/generate', methods=['POST'])
def generate_blocks():
    """Generate blocks (regtest/mining)"""
    data = request.json or {}
    nblocks = str(data.get('blocks', 1))
    address = data.get('address', '')

    if address:
        result = coral_cli('generatetoaddress', nblocks, address)
    else:
        # Try to get an address from loaded wallet
        wallets = coral_cli('listwallets')
        if isinstance(wallets, list) and len(wallets) > 0:
            cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={wallets[0]}', 'getnewaddress']
            try:
                addr_result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
                if addr_result.returncode == 0:
                    address = addr_result.stdout.strip()
                    result = coral_cli('generatetoaddress', nblocks, address)
                else:
                    return jsonify({'error': f'Could not get address: {addr_result.stderr}'})
            except Exception as e:
                return jsonify({'error': str(e)})
        else:
            error_msg = wallets.get('error', 'Unknown error') if isinstance(wallets, dict) else 'No wallet loaded'
            return jsonify({'error': f'No wallet loaded. {error_msg}. Create a wallet first.'})

    return jsonify({'result': result, 'address': address})

@app.route('/api/mining/setgenerate', methods=['POST'])
def set_generate():
    """Start/stop CPU mining"""
    data = request.json or {}
    generate = data.get('generate', False)
    threads = str(data.get('threads', 1))

    if generate:
        result = coral_cli('setgenerate', 'true', threads)
    else:
        result = coral_cli('setgenerate', 'false')

    return jsonify({'result': result, 'generating': generate})

@app.route('/api/network/hashrate')
def get_network_hashrate():
    """Get network hash rate"""
    result = coral_cli('getnetworkhashps')
    return jsonify({'hashrate': result})

# ============== NEW COMPREHENSIVE ENDPOINTS ==============

@app.route('/api/rpc', methods=['POST'])
def execute_rpc():
    """Execute arbitrary RPC command"""
    data = request.json or {}
    command = data.get('command', '').strip()
    if not command:
        return jsonify({'error': 'No command provided'})

    # Parse command into parts
    parts = command.split()
    if not parts:
        return jsonify({'error': 'Empty command'})

    method = parts[0]
    args = parts[1:] if len(parts) > 1 else []

    result = coral_cli(method, *args)
    return jsonify({'result': result})

@app.route('/api/wallet/<name>/utxos')
def get_utxos(name):
    """Get wallet UTXOs"""
    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'listunspent']
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode == 0:
            return jsonify({'utxos': json.loads(result.stdout)})
        return jsonify({'error': result.stderr.strip()})
    except Exception as e:
        return jsonify({'error': str(e)})

@app.route('/api/wallet/<name>/addresses')
def get_addresses(name):
    """Get wallet addresses"""
    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'listreceivedbyaddress', '0', 'true']
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode == 0:
            addresses = json.loads(result.stdout)
            return jsonify({'addresses': addresses})
        return jsonify({'error': result.stderr.strip()})
    except Exception as e:
        return jsonify({'error': str(e)})

@app.route('/api/wallet/<name>/importprivkey', methods=['POST'])
def import_privkey(name):
    """Import private key into wallet"""
    data = request.json or {}
    privkey = data.get('privkey', '')
    label = data.get('label', '')
    rescan = 'true' if data.get('rescan', True) else 'false'

    if not privkey:
        return jsonify({'error': 'No private key provided'})

    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'importprivkey', privkey, label, rescan]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)  # Long timeout for rescan
        if result.returncode == 0:
            return jsonify({'success': True, 'message': 'Private key imported successfully'})
        return jsonify({'error': result.stderr.strip()})
    except subprocess.TimeoutExpired:
        return jsonify({'error': 'Import timed out (rescan may still be in progress)'})
    except Exception as e:
        return jsonify({'error': str(e)})

@app.route('/api/wallet/<name>/send', methods=['POST'])
def send_transaction(name):
    """Send coins"""
    data = request.json or {}
    address = data.get('address', '')
    amount = data.get('amount', 0)

    if not address:
        return jsonify({'error': 'No address provided'})
    if not amount or float(amount) <= 0:
        return jsonify({'error': 'Invalid amount'})

    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'sendtoaddress', address, str(amount)]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        if result.returncode == 0:
            return jsonify({'txid': result.stdout.strip()})
        return jsonify({'error': result.stderr.strip()})
    except Exception as e:
        return jsonify({'error': str(e)})

@app.route('/api/loadwallet', methods=['POST'])
def load_wallet():
    """Load an existing wallet"""
    data = request.json or {}
    name = data.get('name', '')
    if not name:
        return jsonify({'error': 'No wallet name provided'})

    result = coral_cli('loadwallet', name)
    return jsonify(result)

@app.route('/api/unloadwallet', methods=['POST'])
def unload_wallet():
    """Unload a wallet"""
    data = request.json or {}
    name = data.get('name', '')
    if not name:
        return jsonify({'error': 'No wallet name provided'})

    result = coral_cli('unloadwallet', name)
    return jsonify(result)

@app.route('/api/listwalletdir')
def list_wallet_dir():
    """List wallets in wallet directory"""
    result = coral_cli('listwalletdir')
    return jsonify(result)

@app.route('/api/recentblocks')
def get_recent_blocks():
    """Get recent blocks"""
    blocks = []
    blockchain = coral_cli('getblockchaininfo')
    if isinstance(blockchain, dict) and 'blocks' in blockchain:
        height = blockchain['blocks']
        # Get last 10 blocks
        for i in range(min(10, height + 1)):
            block_height = height - i
            blockhash = coral_cli('getblockhash', str(block_height))
            if isinstance(blockhash, str):
                block = coral_cli('getblock', blockhash)
                if isinstance(block, dict):
                    blocks.append({
                        'height': block_height,
                        'hash': blockhash,
                        'time': block.get('time', 0),
                        'tx_count': len(block.get('tx', [])),
                        'size': block.get('size', 0),
                        'weight': block.get('weight', 0)
                    })
    return jsonify({'blocks': blocks})

@app.route('/api/chainstate')
def get_chainstate():
    """Get chain state info"""
    blockchain = coral_cli('getblockchaininfo')
    txoutset = coral_cli('gettxoutsetinfo')
    return jsonify({
        'blockchain': blockchain,
        'txoutset': txoutset
    })

@app.route('/api/networkdetails')
def get_network_details():
    """Get detailed network info"""
    network = coral_cli('getnetworkinfo')
    nettotals = coral_cli('getnettotals')
    return jsonify({
        'network': network,
        'nettotals': nettotals
    })

@app.route('/api/localaddresses')
def get_local_addresses():
    """Get local addresses"""
    network = coral_cli('getnetworkinfo')
    if isinstance(network, dict):
        return jsonify({'localaddresses': network.get('localaddresses', [])})
    return jsonify({'localaddresses': []})

@app.route('/api/banned')
def get_banned():
    """Get banned peers"""
    result = coral_cli('listbanned')
    return jsonify({'banned': result if isinstance(result, list) else []})

@app.route('/api/addnode', methods=['POST'])
def add_node():
    """Add a node"""
    data = request.json or {}
    node = data.get('node', '')
    command = data.get('command', 'add')  # add, remove, onetry

    if not node:
        return jsonify({'error': 'No node address provided'})

    result = coral_cli('addnode', node, command)
    return jsonify({'result': result})

@app.route('/api/disconnectnode', methods=['POST'])
def disconnect_node():
    """Disconnect a node"""
    data = request.json or {}
    address = data.get('address', '')

    if not address:
        return jsonify({'error': 'No address provided'})

    result = coral_cli('disconnectnode', address)
    return jsonify({'result': result})

@app.route('/api/rawmempool')
def get_raw_mempool():
    """Get raw mempool with details"""
    result = coral_cli('getrawmempool', 'true')
    return jsonify(result)

@app.route('/api/decodetx', methods=['POST'])
def decode_tx():
    """Decode raw transaction"""
    data = request.json or {}
    rawtx = data.get('rawtx', '')

    if not rawtx:
        return jsonify({'error': 'No raw transaction provided'})

    result = coral_cli('decoderawtransaction', rawtx)
    return jsonify(result)

@app.route('/api/rawtx/<txid>')
def get_raw_tx(txid):
    """Get raw transaction"""
    result = coral_cli('getrawtransaction', txid, '1')
    return jsonify(result)

@app.route('/api/broadcast', methods=['POST'])
def broadcast_tx():
    """Broadcast raw transaction"""
    data = request.json or {}
    rawtx = data.get('rawtx', '')

    if not rawtx:
        return jsonify({'error': 'No raw transaction provided'})

    result = coral_cli('sendrawtransaction', rawtx)
    return jsonify({'txid': result if isinstance(result, str) else result})

@app.route('/api/debuginfo')
def get_debug_info():
    """Get debug information"""
    info = {
        'blockchain': coral_cli('getblockchaininfo'),
        'network': coral_cli('getnetworkinfo'),
        'memory': coral_cli('getmemoryinfo'),
    }
    return jsonify(info)

@app.route('/api/chaintips')
def get_chain_tips():
    """Get chain tips"""
    result = coral_cli('getchaintips')
    return jsonify({'chaintips': result if isinstance(result, list) else []})

@app.route('/api/deploymentinfo')
def get_deployment_info():
    """Get deployment info"""
    result = coral_cli('getdeploymentinfo')
    return jsonify(result)

@app.route('/api/rpcinfo')
def get_rpc_info():
    """Get RPC info"""
    result = coral_cli('getrpcinfo')
    return jsonify(result)

@app.route('/api/uptime')
def get_uptime():
    """Get node uptime"""
    result = coral_cli('uptime')
    return jsonify({'uptime': result})

@app.route('/api/getblocktemplate')
def get_block_template():
    """Get block template for mining"""
    result = coral_cli('getblocktemplate', '{"rules":["segwit"]}')
    return jsonify(result)

@app.route('/api/estimatesmartfee/<int:blocks>')
def estimate_smart_fee(blocks):
    """Estimate smart fee for confirmation in n blocks"""
    result = coral_cli('estimatesmartfee', str(blocks))
    return jsonify(result)

@app.route('/api/validateaddress/<address>')
def validate_address(address):
    """Validate an address"""
    result = coral_cli('validateaddress', address)
    return jsonify(result)

@app.route('/api/wallet/<name>/signmessage', methods=['POST'])
def sign_message(name):
    """Sign a message with a wallet address"""
    data = request.json or {}
    address = data.get('address', '')
    message = data.get('message', '')

    if not address or not message:
        return jsonify({'error': 'Address and message required'})

    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'signmessage', address, message]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode == 0:
            return jsonify({'signature': result.stdout.strip()})
        return jsonify({'error': result.stderr.strip()})
    except Exception as e:
        return jsonify({'error': str(e)})

@app.route('/api/verifymessage', methods=['POST'])
def verify_message():
    """Verify a signed message"""
    data = request.json or {}
    address = data.get('address', '')
    signature = data.get('signature', '')
    message = data.get('message', '')

    if not address or not signature or not message:
        return jsonify({'error': 'Address, signature, and message required'})

    result = coral_cli('verifymessage', address, signature, message)
    return jsonify({'valid': result})

@app.route('/api/wallet/<name>/dumpprivkey', methods=['POST'])
def dump_privkey(name):
    """Dump private key for an address"""
    data = request.json or {}
    address = data.get('address', '')

    if not address:
        return jsonify({'error': 'No address provided'})

    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'dumpprivkey', address]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode == 0:
            return jsonify({'privkey': result.stdout.strip()})
        return jsonify({'error': result.stderr.strip()})
    except Exception as e:
        return jsonify({'error': str(e)})

@app.route('/api/wallet/<name>/getaddressinfo/<address>')
def get_address_info(name, address):
    """Get address info"""
    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'getaddressinfo', address]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode == 0:
            return jsonify(json.loads(result.stdout))
        return jsonify({'error': result.stderr.strip()})
    except Exception as e:
        return jsonify({'error': str(e)})

@app.route('/api/wallet/<name>/rescanblockchain', methods=['POST'])
def rescan_blockchain(name):
    """Rescan blockchain for wallet transactions"""
    data = request.json or {}
    start_height = data.get('start_height', 0)

    cmd = [CORAL_CLI, f'-datadir={DATADIR}', f'-rpcwallet={name}', 'rescanblockchain', str(start_height)]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=600)  # Long timeout
        if result.returncode == 0:
            return jsonify(json.loads(result.stdout))
        return jsonify({'error': result.stderr.strip()})
    except subprocess.TimeoutExpired:
        return jsonify({'error': 'Rescan timed out (may still be in progress)'})
    except Exception as e:
        return jsonify({'error': str(e)})

if __name__ == '__main__':
    print("Coral Web Dashboard")
    print(f"Using coral-cli: {CORAL_CLI}")
    print(f"Data directory: {DATADIR}")
    print("\nStarting server on http://localhost:5999")
    app.run(debug=True, host='0.0.0.0', port=5999)
