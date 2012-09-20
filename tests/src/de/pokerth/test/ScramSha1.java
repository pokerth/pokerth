/*  PokerTH automated tests.
	Copyright (C) 2010 Lothar May

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

package de.pokerth.test;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.StringTokenizer;

import de.rtner.security.auth.spi.MacBasedPRF;
import de.rtner.security.auth.spi.PBKDF2Engine;
import de.rtner.security.auth.spi.PBKDF2Parameters;

public class ScramSha1 {

	String clientFirstMessageBare;
	SecureRandom random = new SecureRandom();

	public String executeStep1(String username)
	{
		byte bytes[] = new byte[8];
		random.nextBytes(bytes);

		String clientR = Base64Coder.encodeLines(bytes).trim();
		clientFirstMessageBare = "n=" + username + ",r=" + clientR + "\n";
		return "n,," + clientFirstMessageBare;
	}

	public String executeStep2(String password, String serverFirstMessage) throws NoSuchAlgorithmException
	{
		MessageDigest md = MessageDigest.getInstance("SHA-1"); 
		MacBasedPRF hmac = new MacBasedPRF("HMacSha1");

		StringTokenizer st = new StringTokenizer(serverFirstMessage, "=", true);
		st.nextToken("=");
		st.nextToken();
		String serverR = st.nextToken(",");
		st.nextToken();
		st.nextToken("=");
		st.nextToken();
		byte[] serverSalt = Base64Coder.decodeLines(st.nextToken(","));
		st.nextToken();
		st.nextToken("=");
		st.nextToken();
		String iterationCount = st.nextToken(",");

		// SaltedPassword  := Hi(Normalize(password), salt, i)
		PBKDF2Engine engine = new PBKDF2Engine(new PBKDF2Parameters("HMacSHA1", null, serverSalt, Integer.parseInt(iterationCount)));
		byte[] saltedPassword = engine.deriveKey(password, 20);

	    // ClientKey       := HMAC(SaltedPassword, "Client Key")
		hmac.init(saltedPassword);
		byte[] clientKey = hmac.doFinal("Client Key".getBytes());

		// StoredKey       := H(ClientKey)
		byte[] storedKey = md.digest(clientKey);

	    // AuthMessage     := client-first-message-bare + "," +
        // server-first-message + "," +
        // client-final-message-without-proof
		String clientFinalMessage = "c=biws,r=" + serverR;
		String strAuthMessage = clientFirstMessageBare + "," + serverFirstMessage + "," + clientFinalMessage;

		// ClientSignature := HMAC(StoredKey, AuthMessage)
		hmac = new MacBasedPRF("HMacSha1");
		hmac.init(storedKey);
		byte[] clientSignature = hmac.doFinal(strAuthMessage.getBytes());

		// ClientProof     := ClientKey XOR ClientSignature
        for (int i = 0; i < 20; i++)
        {
        	clientKey[i] ^= clientSignature[i];
        }

		String clientProof = Base64Coder.encodeLines(clientKey).trim();
		clientFinalMessage += ",p=" + clientProof;
		return clientFinalMessage;
	}
}
