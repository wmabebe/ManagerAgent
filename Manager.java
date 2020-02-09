import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.DatagramPacket; 
import java.net.DatagramSocket; 
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Scanner;
import java.util.concurrent.TimeUnit;


/**
 * This class starts by, spawning a thread that will determine the liveliness of current agents.
 * It continues on to listen to udp packets awaiting incomming beacons.
 * Upon receiving a new beacon, it spawns another thread to handle the remote agent that sent the beacon.
 * 
 * @author wmabebe
 */
public class Manager 
{ 
	public static HashMap<Integer,Beacon> map = new HashMap<Integer,Beacon>();
	public static int MIN_INTERVAL = 5;
	public static void main(String[] args) throws IOException 
	{ 
		
		DatagramSocket ds = new DatagramSocket(8080); 
		byte[] receive = new byte[32]; 

		DatagramPacket DpReceive = null;
		boolean firstTime = true;
		Cmd cmd;
		new Live().start();
		System.out.println("Manager listening to beacons...");
		while (true) 
		{ 

			// Step 2 : create a DatgramPacket to receive the data. 
			DpReceive = new DatagramPacket(receive, receive.length); 

			// Step 3 : revieve the data in byte buffer. 
			ds.receive(DpReceive); 

			//System.out.println("Client:-" + data(receive));
			//System.out.println(Arrays.toString(receive));
			Beacon b = new Beacon(receive);
					
			MIN_INTERVAL = (b.getInterval() * 2) < MIN_INTERVAL ? (b.getInterval() * 2) : MIN_INTERVAL;
			if (!map.containsKey(b.getId())){
				System.out.println("\n==================================");
				System.out.println("NEW " + b + " detected!");
				System.out.println("Beacon ID: " + b.getId());
				System.out.println("Beacon Startup: " + b.getStartup());
				System.out.println("Beacon Interval: " + b.getInterval());
				System.out.println("Beacon Ip: " + Arrays.toString(b.getIp()));
				System.out.println("Beacon Port: " + b.getPort());
				System.out.println("=====================================\n");
				new Cmd(b.getId(),b.getIp(),b.getPort()).start();
	
			}
			else if (map.get(b.getId()) == null)
				System.out.println(b + " resurrected!");
			else
				System.out.println(b + " beacon...");
			map.put(b.getId(),b);
			
			// Clear the buffer after every message. 
			receive = new byte[32];
			
		} 
	} 
}

/**
 * This thread sends commands to the remote agent that sent a beacon.
 *
 * @author wmabebe
 */
class Cmd extends Thread{
	private int id,port;
	private String ip = "";

	/**
	 * Construct the thread using beacon id, beacon ip, and beacon port
	 */
	public Cmd(int bid,int[] bip,int bport) {
		this.port = bport;
		this.id = bid;
		for (int i=0;i<bip.length;i++) {
			ip += bip[i];
			if (i < bip.length - 1)
				ip += ".";
		}
			
	}
	
	/**
	 * This method prints out the socket information
	 */
	static void printSocketInfo(Socket s) {
		System.out.print("Manager (Local) IP: " + s.getLocalAddress() + ":"
				+ s.getLocalPort()+ "\t");
		System.out.println("Agent (Remote) IP: " + s.getRemoteSocketAddress());
	}
	
	/**
	 * This run method flushes commands 'OS' and 'TIME' respectively.
	 * It intermittently listens to incomming responses and prints them out.
	 */
	public void run() {
		Socket socket;
		try {
			socket = new Socket(ip, port);
			//printSocketInfo(socket);
			PrintWriter out = new PrintWriter(new BufferedOutputStream(socket.getOutputStream()));
			Scanner in = new Scanner(new BufferedInputStream(socket.getInputStream()));
			DataInputStream dis = new DataInputStream(socket.getInputStream());
			out.println("OS");
			out.flush();
			String response = in.nextLine();
			System.out.println("Agent " + id + " OS: " + response);
			out.println("TIME");
			out.flush();
			byte[] reply = new byte[1024];
			//int bytesRead = dis.read(reply);
			System.out.println("Read " + bytesRead + " bytes!");
			//is.read(reply);
			byte[] slice = Arrays.copyOfRange(reply,0, 4);
			response = ByteBuffer.wrap(slice).order(java.nio.ByteOrder.LITTLE_ENDIAN).getInt() + "";
			System.out.println("Agent " + id + " TIME: " + response);
			out.close();
			socket.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}

/**
 * This thread examines the last last beacons of every active remote agent.
 * If a becon is found to be stale, it indicates the death of the remote sending agent
 * and the beacon is removed from the map of beacons.
 *
 * @author wmabebe
 */
class Live extends Thread{
	public void run() {
		while (true) {
			
			for(Entry<Integer, Beacon> entry: Manager.map.entrySet()) {
				if (entry.getValue() != null && entry.getValue().isStale()) {
					System.out.println(entry.getValue() + " has died!");
					Manager.map.put(entry.getKey(),null);
				}
			}
			
			try {
				Thread.sleep(Manager.MIN_INTERVAL);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

		}
	}
}

/**
 * Beacon class encapsulates an incomming udp packet.
 *
 * @author wmabebe
 */

class Beacon{
	/** These values are unpacked from the incomming beacon */
	private int id, startup, interval, cmdPort;
	/** This value captures the IP of the remote Agent */
	private int[] ip;
	/** This value captures time of instantiation in seconds */
	private long received;
	/** This value is used to mask bits inorder to convert bytes from C char to hava int */
	public final static int MASK = 0xff;
	
	/**
	 * Construct a beacon instance.
	 * 
	 * @param data a byte array that is sliced to instantiate instance variables
	 */
	public Beacon(byte[] data) {
		if (data.length < 32) {
			throw new IllegalArgumentException();
		}
		byte[] slice = Arrays.copyOfRange(data,0, 4); 
		this.id = ByteBuffer.wrap(slice).order(java.nio.ByteOrder.LITTLE_ENDIAN).getInt();
		slice = Arrays.copyOfRange(data,4, 8); 
		this.startup = ByteBuffer.wrap(slice).order(java.nio.ByteOrder.LITTLE_ENDIAN).getInt();
		slice = Arrays.copyOfRange(data,8, 12);
		this.interval = ByteBuffer.wrap(slice).order(java.nio.ByteOrder.LITTLE_ENDIAN).getInt();
		slice = Arrays.copyOfRange(data,12, 16);
		this.cmdPort = ByteBuffer.wrap(slice).order(java.nio.ByteOrder.LITTLE_ENDIAN).getInt();
		
		this.ip = new int[4];
		slice = Arrays.copyOfRange(data,16, 20);
		ByteBuffer bb = ByteBuffer.wrap(slice);
		bb.order( ByteOrder.LITTLE_ENDIAN);
		int i=0;
		//for (int i=0; i<4 ;i++) {
		while( bb.hasRemaining()) {
		   this.ip[i++] = bb.get() & MASK;
		}

		this.received = System.currentTimeMillis() / 1000;
	}

	/**
	 * Get id value
	 * @return the id of this beacon
	 */
	public int getId() {
		return this.id;
	}

	/**
	 * Get startup time of this beacon
	 * @return the startup time of the beacon
	 */	
	public int getStartup() {
		return this.startup;
	}
	
	/**
	 * Get interval of beacon
	 * @return the beacon's interval in seconds
	 */
	public int getInterval() {
		return this.interval;
	}
	
	/**
	 * Get the port of the remote agent
	 * @return the port number
	 */
	public int getPort() {
		return this.cmdPort;
	}
	
	/**
	 * Get the IP address of the remote agent
	 * @return the IP address
	 */
	public int[] getIp() {
		return this.ip;
	}
	
	/**
	 * Get the received time
	 * @return received time
	 */
	public long getReceived() {
		return received;
	}
	
	/**
	 * Calculate whether or not the beacon is overdue.
	 * An overdue becon indicates that it hasn't been replaced by a new one
	 * for more than the interval necessary. It could indicate the death of
	 * the remote agent that is sending the beacons.
	 * @return True if the beacon is overdue.
	 */
	public boolean isStale() {
		return (System.currentTimeMillis() / 1000) - received > (2 * interval);
	}
	
	/**
	 * Return toString representation of beacon.
	 * @return String representation.
	 */
	public String toString(){
		return "Agent " + this.id;
	}
}
