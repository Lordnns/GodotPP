use crate::{GamePeer, GameNetworkEvent, GameConnection, GameStream, GameStreamReliability, protocols::QuicBackend};
use std::ffi::{c_void, CStr};
use std::os::raw::c_char;

#[unsafe(no_mangle)]
pub extern "C" fn qns_create_peer() -> *mut c_void {
    let backend = QuicBackend::new();
    let peer = Box::new(GamePeer::new(backend));
    Box::into_raw(peer) as *mut c_void
}

#[unsafe(no_mangle)]
pub extern "C" fn qns_connect(peer_ptr: *mut c_void, ip: *const c_char, port: u16) {
    if peer_ptr.is_null() { return; }
    let peer = unsafe { &mut *(peer_ptr as *mut GamePeer) };
    let ip_str = unsafe { CStr::from_ptr(ip) }.to_str().unwrap_or("127.0.0.1");
    let _ = peer.connect(ip_str, port);
}

#[unsafe(no_mangle)]
pub extern "C" fn qns_destroy_peer(peer_ptr: *mut c_void) {
    if peer_ptr.is_null() { return; }
    unsafe {
        let mut peer = Box::from_raw(peer_ptr as *mut GamePeer);
        let _ = peer.shutdown();
    }
}

/// Polls the event queue. Returns a pointer to a Bytes object if a message was received.
#[unsafe(no_mangle)]
pub extern "C" fn qns_poll_event(
    peer_ptr: *mut c_void,
    out_type: *mut u8,             // 0=None, 1=Connected, 2=Disconnected, 3=Message
    out_uuid: *mut u8,             // 16-byte array for the UUID
    out_stream_id: *mut u16,       // For identifying the channel
    out_data_ptr: *mut *const u8,  // Pointer to the raw payload
    out_data_len: *mut usize,      // Length of the payload
) -> *mut c_void {
    if peer_ptr.is_null() { return std::ptr::null_mut(); }
    let peer = unsafe { &mut *(peer_ptr as *mut GamePeer) };

    unsafe { *out_type = 0; *out_stream_id = 0; *out_data_len = 0; }

    if let Ok(Some(event)) = peer.poll() {
        match event {
            GameNetworkEvent::Connected(conn) => {
                unsafe {
                    *out_type = 1;
                    std::ptr::copy_nonoverlapping(conn.connection_id.as_bytes().as_ptr(), out_uuid, 16);
                }
                return 1 as *mut c_void;
            }
            GameNetworkEvent::Disconnected(conn) => {
                unsafe {
                    *out_type = 2;
                    std::ptr::copy_nonoverlapping(conn.connection_id.as_bytes().as_ptr(), out_uuid, 16);
                }
                return 1 as *mut c_void;
            }
            GameNetworkEvent::Message { connection, stream, data } => {
                unsafe {
                    *out_type = 3;
                    std::ptr::copy_nonoverlapping(connection.connection_id.as_bytes().as_ptr(), out_uuid, 16);
                    *out_stream_id = stream.stream_id;
                    *out_data_ptr = data.as_ptr();
                    *out_data_len = data.len();
                }
                return Box::into_raw(Box::new(data)) as *mut c_void;
            }
            _ => {}
        }
    }
    std::ptr::null_mut()
}

#[unsafe(no_mangle)]
pub extern "C" fn qns_listen(peer_ptr: *mut c_void, port: u16) {
    if peer_ptr.is_null() { return; }
    let peer = unsafe { &mut *(peer_ptr as *mut GamePeer) };
    let _ = peer.listen(port);
}

#[unsafe(no_mangle)]
pub extern "C" fn qns_free_bytes(bytes_handle: *mut c_void) {
    if bytes_handle.is_null() || bytes_handle == 1 as *mut c_void {
        return; // DO NOT try to free the dummy pointer!
    }
    unsafe {
        // Only free if it's an actual Boxed Bytes object
        let _ = Box::from_raw(bytes_handle as *mut bytes::Bytes);
    }
}

/// SENDS a packet from C++ through the Rust Backend
#[unsafe(no_mangle)]
pub extern "C" fn qns_send(
    peer_ptr: *mut c_void,
    uuid_bytes: *const u8,
    stream_index: u16,
    is_reliable: bool,
    data_ptr: *const u8,
    data_len: usize,
) {
    if peer_ptr.is_null() || uuid_bytes.is_null() || data_ptr.is_null() || data_len == 0 { return; }

    let peer = unsafe { &mut *(peer_ptr as *mut GamePeer) };

    // 1. Reconstruct UUID
    let mut uuid_arr = [0u8; 16];
    unsafe { std::ptr::copy_nonoverlapping(uuid_bytes, uuid_arr.as_mut_ptr(), 16); }
    let conn = GameConnection { connection_id: uuid::Uuid::from_bytes(uuid_arr) };

    // 2. Format Stream
    let rel = if is_reliable { GameStreamReliability::Reliable } else { GameStreamReliability::Unreliable };
    let stream = GameStream::new(stream_index, rel);

    // 3. Copy bytes safely and send
    let data_slice = unsafe { std::slice::from_raw_parts(data_ptr, data_len) };
    let bytes = bytes::Bytes::copy_from_slice(data_slice);

    let _ = peer.send(&conn, &stream, bytes);
}