// DualOStream.h
// Constructs a cout replacement which tees the output to two ostreams
// Used for printing the same content to both console and logfile.
//
// Copyright (c) 2019 Matthew Breit - matt.breit@baslerweb.com or matt.breit@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Based on public domain code from http://wordaligned.org/articles/cpp-streambufs

#ifndef DUALOSTREAM_H
#define DUALOSTREAM_H

#include <iostream>
#include <chrono>
#include <ctime>
#include <string>
#include <mutex>
#include <iomanip>

namespace DualOStream
{
	class CTeeBuf : public std::streambuf
	{
	public:
		// Construct a streambuf which tees output to both input
		// streambufs.
		CTeeBuf(std::streambuf * sb1, std::streambuf * sb2) : sb1(sb1), sb2(sb2)
		{
		}
		bool timeStampEnabledStream1 = false;
		bool timeStampEnabledStream2 = false;
		std::string timeStamp = "";
		bool forceMessage = false;
		std::string forcedMessage = "";

	private:
		// Get the current timestamp in relation to the start of the clock.
		// The clock starts on the first call of GetTimeStamp() (essentially in this application, the first line written to the stream starts the clock)
		const std::string GetTimeStamp()
		{
			if (clockStarted == false)
			{
				start = std::chrono::high_resolution_clock::now();
				clockStarted = true;
			}
			now = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> time_span = now - start;
			std::string strTimeStamp = "[";

			std::chrono::system_clock::time_point real = std::chrono::system_clock::now();
			time_t tt = std::chrono::system_clock::to_time_t(real);
			tm local_tm = *localtime(&tt);
			strTimeStamp.append(std::to_string(local_tm.tm_year + 1900));
			strTimeStamp.append("-");
			strTimeStamp.append(std::to_string(local_tm.tm_mon + 1));
			strTimeStamp.append("-");
			strTimeStamp.append(std::to_string(local_tm.tm_mday));
			strTimeStamp.append("|");
			strTimeStamp.append(std::to_string(local_tm.tm_hour));
			strTimeStamp.append(":");
			strTimeStamp.append(std::to_string(local_tm.tm_min));
			strTimeStamp.append(":");
			strTimeStamp.append(std::to_string(local_tm.tm_sec));
			strTimeStamp.append("|");
			strTimeStamp.append(std::to_string(time_span.count()));
			strTimeStamp.append("] ");
			return strTimeStamp;
		}

		// This tee buffer has no buffer. So every character "overflows"
		// and can be put directly into the teed buffers.
		virtual int overflow(int c)
		{
			if (c == EOF)
			{
				return !EOF;
			}
			else
			{
				std::ostream str1(sb1);
				std::ostream str2(sb2);

				if (forceMessage == true)
				{
					newline = true;
					str1 << "\n";
					str2 << "\n";
				}

				if (newline == true)
				{
					if (timeStampEnabledStream1 == true || timeStampEnabledStream2 == true)
					{
						timeStamp = GetTimeStamp();

						if (timeStampEnabledStream1 == true)
							str1 << std::setw(32) << std::left << timeStamp;
						if (timeStampEnabledStream2 == true)
							str2 << std::setw(32) << std::left << timeStamp;
					}

					if (forceMessage == true)
					{
						str1 << forcedMessage;
						str2 << forcedMessage;

						forceMessage = false;
						c = '\n';
					}
				}

				if (traits_type::to_char_type(c) == '\n')
					newline = true;
				else
					newline = false;

				int const r1 = sb1->sputc(c);
				int const r2 = sb2->sputc(c);

				return r1 == EOF || r2 == EOF ? EOF : c;
			}
		}

		// Sync both teed buffers.
		virtual int sync()
		{
			int const r1 = sb1->pubsync();
			int const r2 = sb2->pubsync();
			return r1 == 0 && r2 == 0 ? 0 : -1;
		}
	private:
		std::streambuf * sb1;
		std::streambuf * sb2;
		bool clockStarted = false;
		std::chrono::high_resolution_clock::time_point start;
		std::chrono::high_resolution_clock::time_point now;
		bool newline = true;
	};

	class DStream : public std::ostream
	{
	public:
		// Construct an ostream which tees output to the supplied
		// ostreams.
		inline DStream(std::ostream & o1, std::ostream & o2) : std::ostream(&tbuf), tbuf(o1.rdbuf(), o2.rdbuf())
		{
		}

		// another constructor allowing the user to add timestamps to either or both of the streams.
		inline DStream(std::ostream & o1, std::ostream & o2, bool enableTimeStampStream_1, bool enableTimeStampStream_2) : std::ostream(&tbuf), tbuf(o1.rdbuf(), o2.rdbuf())
		{
			if (enableTimeStampStream_1 == true)
				EnableTimeStampStream1();
			else
				DisableTimeStampStream1();

			if (enableTimeStampStream_2 == true)
				EnableTimeStampStream2();
			else
				DisableTimeStampStream2();
		}

		inline void EnableTimeStampStream1()
		{
			tbuf.timeStampEnabledStream1 = true;
		}
		inline void EnableTimeStampStream2()
		{
			tbuf.timeStampEnabledStream2 = true;
		}
		inline void DisableTimeStampStream1()
		{
			tbuf.timeStampEnabledStream1 = false;
		}
		inline void DisableTimeStampStream2()
		{
			tbuf.timeStampEnabledStream2 = false;
		}
		inline std::string GetLastTimeStamp()
		{
			return tbuf.timeStamp;
		}
		inline void ForceMessage(std::string message)
		{
			message.append("\n");
			tbuf.forcedMessage = message;
			tbuf.forceMessage = true;
			tbuf.sputc('\n');
			
			while (tbuf.forceMessage == true)
			{
				// wait until message is forced
			}
		}
	private:
		CTeeBuf tbuf;
	};
}
#endif

// Implementation Example:
// dout replaces cout. It will direct all dout << to both the console and the log file.
// std::ofstream mylog;
// mylog.open("myfilename.txt");
// DualStream::DStream dout(std::cout, mylog);
// dout << "hello world!" << endl;
// dout.ForceMessage("hello world!");