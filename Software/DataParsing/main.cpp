/*
 * main.cpp
 *
 * Version: 1.00
 * Date: 25/06/2024
 * Author: Caterina Morgavi
 * caterina.morgavi@studenti.unimi.it
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY
 * APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
 * HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
 * IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
 * ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
 */


#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <stdio.h>


#define SEPCHAR char(32)

using namespace std;

typedef struct measurement
{
  double time;
  double cal1;
  double cal2;
};


int countLines(const char *_filename);
void SetColor (int textColor);
void ResetColor();


int main(int argc, char **argv)
{
  /* Stream */
  ofstream outfile;
  ifstream myfile;
  string rawline;
  /* Line parsing */
  int lines;
  bool validstring_flag = true; // Valid lines must contain only numbers (at least one) or SEPCHAR
  /* Data parsing */
  int pos;
  double lim_sup = 3700.0;
  string remainder;
  measurement rawMeasurement;
  /* Compute tof */
  double calcount;
  double tof;
  /* Results */
  int stopOUTcount = 0;
  int stopINcount = 0;

  if (argc < 1)
  {
    SetColor(31);
    cerr << "Error, expected at least one argument! Usage: <rawdata filename> <STOP limit>" << endl;
    ResetColor;
    return -1;
  }
  if (argc > 3)
  {
    SetColor(31);
    cerr << "Error, too many arguments." << endl;
    ResetColor;
    return -1;
  }

  try
  {
    lines = countLines(argv[1]);
    if (argc == 3)
    {
      lim_sup = stod(argv[2]);
      if (lim_sup >=  3926.0) {
        lim_sup = 3700;
        SetColor(33);
        cout << "STOP limit must be < 3926ns, STOP limit set to 3700ns." << endl;
        ResetColor();
      }else{
        SetColor(33);
        cout << "STOP limit set to " << lim_sup << "ns, STOPs above " << lim_sup << "ns will be ignored." << endl;
        ResetColor();
      }
    }
    else
    {
      SetColor(33);
      cout << "STOP limit set to 3700ns, STOPs above 3700ns will be ignored." << endl;
      ResetColor();
    }
  }
  catch (const invalid_argument &e)
  {
    cerr << e.what() << '\n';
  }
  catch (const logic_error &e)
  {
    cerr << e.what() << '\n';
  }

  SetColor(33);
  cout << "Lines: " << lines << endl;
  ResetColor();

  myfile.open(argv[1]);
  outfile.open(static_cast<string>("OUT") + static_cast<string>(argv[1]));

  for (int j = 0; j < lines; j++)
  {
    getline(myfile, rawline);
    validstring_flag = true;

    // Validate line characters
    if (rawline.length() <= 10)
    {
      validstring_flag = false;
    }
    else
    {
      for (unsigned int i = 0; i < (rawline.length() -1); i++)
      {
        if (((rawline[i] < '0') || (rawline[i] > '9')) && (rawline[i] != SEPCHAR))
        {
          validstring_flag = false;
          break;
        }
      }
      //cout << rawline.length() << endl;
    }

    if (validstring_flag == false)
    {
      SetColor(31);
      cout << "Ignored";
      ResetColor();
      cout << " line " <<  j <<  " >>> " << rawline << endl;
    }
    else
    {
      try
      {
        /* Data parsing */
        pos = rawline.find(SEPCHAR);
        rawMeasurement.time = (stod(rawline.substr(0, pos)) * pow(2, 16));
        remainder = rawline.substr(pos + 1);

        pos = remainder.find(SEPCHAR);
        rawMeasurement.time = rawMeasurement.time + (stod(remainder.substr(0, pos)) * pow(2, 8));
        remainder = remainder.substr(pos + 1);

        pos = remainder.find(SEPCHAR);
        rawMeasurement.time = rawMeasurement.time + stod(remainder.substr(0, pos));
        remainder = remainder.substr(pos + 1);

        // Load rawMeasurement.cal1
        pos = remainder.find(SEPCHAR);
        rawMeasurement.cal1 = (stod(remainder.substr(0, pos)) * pow(2, 16));
        remainder = remainder.substr(pos + 1);
        pos = remainder.find(SEPCHAR);
        rawMeasurement.cal1 = rawMeasurement.cal1 + (stod(remainder.substr(0, pos)) * pow(2, 8));
        remainder = remainder.substr(pos + 1);
        pos = remainder.find(SEPCHAR);
        rawMeasurement.cal1 = rawMeasurement.cal1 + stod(remainder.substr(0, pos));
        remainder = remainder.substr(pos + 1);

        // Load rawMeasurement.cal2
        pos = remainder.find(SEPCHAR);
        rawMeasurement.cal2 = (stod(remainder.substr(0, pos)) * pow(2, 16));
        remainder = remainder.substr(pos + 1);
        pos = remainder.find(SEPCHAR);
        rawMeasurement.cal2 = rawMeasurement.cal2 + (stod(remainder.substr(0, pos)) * pow(2, 8));
        remainder = remainder.substr(pos + 1);
        pos = remainder.find(SEPCHAR);
        rawMeasurement.cal2 = rawMeasurement.cal2 + stod(remainder.substr(0, pos));
        remainder = remainder.substr(pos + 1);

        /* Compute tof */
        calcount = (rawMeasurement.cal2 - rawMeasurement.cal1) / (9.0);
        tof = (rawMeasurement.time * (1.0 / 8000000.0) / (calcount)) * pow(10.0, 9.0);

        /* Remove stops from data */
        if (tof <= lim_sup)
        {
          stopINcount++;
          outfile << tof << endl;
        }
        else
        {
          stopOUTcount++;
        }
      }
      catch (const invalid_argument &e)
      {
        myfile.close();
        outfile.close();
        cerr << e.what() << '\n';
      }
    }
  }
  
  SetColor(33);
  cout << "STOPs out of range: " << stopOUTcount << endl;
  cout << "STOPs in range: " << stopINcount << endl;
  ResetColor();

  myfile.close();
  outfile.close();

  return 0;
}

int countLines(const char *_filename)
{
  int count = 0;
  string line;
  ifstream myfile;

  if (myfile.is_open())
  {
    throw invalid_argument("File is already open");
  }
  else
  {
    myfile.open(_filename);
  }

  if (!myfile.is_open())
  {
    throw logic_error("File failed to open");
  }
  while (1)
  {
    getline(myfile, line);

    if (myfile.eof())
    {
      break;
    }

    ++count;
  }

  myfile.close();
  return count;
}

void SetColor(int textColor)
{
  cout << "\033[" << textColor << "m";
};

void ResetColor()
{
  cout << "\033[0m";
};