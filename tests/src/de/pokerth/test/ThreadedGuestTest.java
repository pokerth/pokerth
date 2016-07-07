package de.pokerth.test;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.IOException;
import java.net.Socket;

import org.junit.Assert;
import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.AnnounceMessage;
import de.pokerth.protocol.ProtoBuf.AnnounceMessage.ServerType;
import de.pokerth.protocol.ProtoBuf.ErrorMessage;
import de.pokerth.protocol.ProtoBuf.InitAckMessage;
import de.pokerth.protocol.ProtoBuf.InitMessage;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;

/**
 * Description:
 * 
 *  This test runs several guests users in PokerTH Server. 
 *  Starts N threads thats connects to server "at once": There is a delay between each connection, to avoid error 133
 *   (NOTE: I didn't succed avoid this message. So, it's better to disable "ServerBruteForceProtection" in config.xml)
 *  Once connected, each thread waits until all other connects to simulate N users at once 
 * 
 *  When all threads are connected, each thread gets unlocked and close connections, in an orderly way.  
 * 
 * 
 * Running: 
 * 
 *  For OFFICIAL_SERVER, runs with type='O', passwd is ignored  
 *  For DEDICATED_SERVER: 
 * 	 -	Server must be faked to accept guest connections (aren't allowed by default). 
 * 		Method ServerLobbyThread::HandleNetPacketInit() must be changed 	 
 *   -	runs this test, with type='S' and passwd for dedicated server 
 * 
 * Purpose: 
 * 
 *  Final purpose of this test, is check whether a guest connection isn't permitted  
 *   once the maximum number of guests players allowed is reached 
 *  To achieve that, N (NUM_THREADS) must be SERVER_MAX_GUEST_USERS + 1. That value is defined in ServerLobbyThread 
 *  In that case, test will fail, because it's not supposed to receive that error message. 
 *  Test must be corrected: 
 * 	 - Last thread launched (or the one that makes SERVER_MAX_GUEST_USERS + 1, and next ones), must receive an error
 *   - To do that,  msg.hasErrorMessage() must be true, and reason (msg.getErrorMessage().getErrorReason()) 
 *  	must be ERR_NET_SERVER_FULL (error gameIsFull) 
 * 
 * 
 * TODO: 
 * 
 *  This test must be modified to adapt to:  
 *   - Java8 (much simple threads) Runnable name = () -> {code}  
 *   - Modern concurrency design: using Executors, Barriers and CountDownLatch 
 * 
 * 
 * @author albmed
 *
 */
public class ThreadedGuestTest {

	public static final String type = "S"; 
	public static final String passwd = "PasswdServer";
	
	public static final int NUM_THREADS = 7; 
	
	@Test
	public void threadedGuestTest() throws InterruptedException { 
		Assert.assertNotNull(type);
		if (!type.matches("[OoSs]")) Assert.fail("No valid option");
		
		if (type.equalsIgnoreCase("S") && (passwd == null || passwd.trim().length() == 0))
			Assert.fail("Server Passwd needed on Dedicated server");

		ThreadGuest[] threads = new ThreadGuest[NUM_THREADS]; 
		
		for (int i = 0; i < threads.length; i++) {
			threads[i] = new ThreadGuest("Guest" + String.format("%05d",i), type, passwd, new Object()); 
			threads[i].start(); 
			
			Thread.sleep(1500L); // Must wait over a second, before start next thread, to avoid error 133 (ERR_NET_INIT_BLOCKED)  
		} 
		
		Thread.sleep(1000L);
		
		for (int i = 0; i < threads.length; i++) { 
			Object lock = threads[i].getLock(); 
			synchronized (lock) {
				lock.notify();
			}
			try {
				threads[i].getThread().join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			} 
		}
		
	}
	
	public static PokerTHMessage receiveMessage(Socket s) throws Exception {
		byte[] header = new byte[4];
		s.getInputStream().read(header);
		int size = 0;
		for (int i = 0; i < 4; i++) {
			size <<= 8;
			size |= (int)header[i];
		}
		byte[] data = new byte[size];
		s.getInputStream().read(data);
		return PokerTHMessage.parseFrom(data);
	}

	public static void sendMessage(PokerTHMessage msg, Socket s) throws Exception {
		int size = msg.getSerializedSize();
		byte[] header = new byte[4];
		for (int i = 0; i < 4; i++)
		{
			header[i] = (new Integer(size >>> 24)).byteValue();
			size <<= 8;
		}
		s.getOutputStream().write(header);
		s.getOutputStream().write(msg.toByteArray());
	}

	
	public static void failOnErrorMessage(PokerTHMessage msg) {
		if (msg.hasErrorMessage())
		{
			ErrorMessage error = msg.getErrorMessage();
			fail("Received error: " + error.getErrorReason().toString());
		}
	}

}

class ThreadGuest implements Runnable {

	public final int PROTOCOL_VERSION_MAJOR = 5;
	public final int PROTOCOL_VERSION_MINOR = 1;

	Thread t; 
	Object lock; 
	String name; 
	String type; 
	Socket s; 
	String passwd; 
	
	public ThreadGuest(String _name, String _type, String _passwd, Object _lock) { 
		this.name = _name;
		if (_lock == null) lock = new Object(); 
		this.lock = _lock;
		this.type = _type;
		this.passwd = _passwd; 
	}
	
	public Object getLock() { return this.lock; } 
	public Thread getThread() { return this.t; } 
	
	public void start() { 
		if (t == null) {
			t = new Thread(this); 
			t.start(); 
		}
	}
	
	@Override
	public void run() {
		int playerId = connect();
		
		System.out.println("[" + System.currentTimeMillis() + " - "  + name + "]: PlayerId: " + playerId + " connected");
		
		// Wait to make all guests users connected at once 
		synchronized (lock) {
			try {
				lock.wait();
			} catch (InterruptedException e) {
				e.printStackTrace();
			} 
		}

		System.out.println("[" + System.currentTimeMillis() + " - "  + name + "]: PlayerId: " + playerId + " released");
		
		// Close conn
		if (s != null && !s.isClosed()) try {s.close();} catch (IOException e) {}
	}
	
	public int connect() { 
		Exception excep = null; 
		int playerId = -1;

		try { 
			s = new Socket("localhost", 7234);
			PokerTHMessage msg = ThreadedGuestTest.receiveMessage(s);
			Assert.assertTrue(msg.hasAnnounceMessage()); 
			
			if (type.equalsIgnoreCase("S")) Assert.assertTrue(msg.getAnnounceMessage().getServerType() == ServerType.serverTypeInternetNoAuth);
			
			AnnounceMessage.Version requestedVersion = AnnounceMessage.Version.newBuilder()
					.setMajorVersion(PROTOCOL_VERSION_MAJOR)
					.setMinorVersion(PROTOCOL_VERSION_MINOR)
					.build();
				
			InitMessage init; 
			if (type.equalsIgnoreCase("S")) {
				init = InitMessage.newBuilder()
						.setBuildId(0)
						.setLogin(InitMessage.LoginType.unauthenticatedLogin)
						.setRequestedVersion(requestedVersion)
						.setNickName(name).setAuthServerPassword(passwd)
						.build();
			}
			else {
				init = InitMessage.newBuilder()
						.setBuildId(0)
						.setLogin(InitMessage.LoginType.guestLogin)
						.setRequestedVersion(requestedVersion)
						.setNickName(name)
						.build();
			}
			
			msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_InitMessage)
				.setInitMessage(init)
				.build();
			
			ThreadedGuestTest.sendMessage(msg, s);
			
			msg = ThreadedGuestTest.receiveMessage(s);
			ThreadedGuestTest.failOnErrorMessage(msg);
			
			if (msg.hasInitAckMessage() && msg.getMessageType() == PokerTHMessageType.Type_InitAckMessage) {
				InitAckMessage initAck = msg.getInitAckMessage();
				assertTrue(initAck.getYourPlayerId() != 0L);
				assertTrue(!initAck.hasYourAvatarHash());
				playerId = initAck.getYourPlayerId();
			}
			else {
				ThreadedGuestTest.failOnErrorMessage(msg);
				Assert.fail("Invalid message.");
			}
		}
		catch (Exception e) { 
			excep = e; 
			e.printStackTrace();
		}
		finally { 
			if (excep != null) {
				if (s != null) try {s.close();} catch (IOException e) {}
			}
		}
		
		return playerId;
	}
	
}
