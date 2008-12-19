/***************************************************************************
 *   Copyright (C) 2008 by Lothar May                                      *
 *                                                                         *
 *   This file is part of pokerth_console.                                 *
 *   pokerth_console is free software: you can redistribute it and/or      *
 *   modify it under the terms of the GNU Affero General Public License    *
 *   as published by the Free Software Foundation, either version 3 of     *
 *   the License, or (at your option) any later version.                   *
 *                                                                         *
 *   pokerth_console is distributed in the hope that it will be useful,    *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the                                *
 *   GNU Affero General Public License along with pokerth_console.         *
 *   If not, see <http://www.gnu.org/licenses/>.                           *
 ***************************************************************************/

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using zlib;

namespace pokerth_console
{
	class ZlibHelper
	{
		public static void UncompressFile(string compressedFile, string outputFile)
		{
			ZStream zStream = new ZStream();
			zStream.inflateInit();
			FileStream inputStream = File.Open(compressedFile, FileMode.Open,FileAccess.Read);
			FileStream outputStream = File.Open(outputFile, FileMode.Create, FileAccess.Write);
			const int InBufSize = 4096;
			const int OutBufSize = 8192;
			byte[] inBuf = new byte[InBufSize];
			byte[] outBuf = new byte[OutBufSize];
			int bytesRead;
			int ret;

			do
			{
				bytesRead = inputStream.Read(inBuf, 0, InBufSize);
				if (bytesRead == 0)
					throw new IOException("Unexpected end-of-file during uncompression.");

				zStream.next_in = inBuf;
				zStream.next_in_index = 0;
				zStream.avail_in = bytesRead;
				do
				{
					zStream.next_out = outBuf;
					zStream.next_out_index = 0;
					zStream.avail_out = OutBufSize;
					ret = zStream.inflate(zlibConst.Z_NO_FLUSH);

					if (ret != zlibConst.Z_OK && ret != zlibConst.Z_STREAM_END)
						throw new IOException("Error uncompressing file: " + zStream.msg);
					outputStream.Write(outBuf, 0, OutBufSize - zStream.avail_out);
				} while (zStream.avail_out == 0);
			} while (ret != zlibConst.Z_STREAM_END);
			zStream.inflateEnd();
			// Close files here, because otherwise it might take some time.
			inputStream.Close();
			outputStream.Close();
		}
	}
}
