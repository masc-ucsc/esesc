#if 0
/*------------------------------------------------------------
 *                              CACTI 6.5
 *         Copyright 2008 Hewlett-Packard Development Corporation
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein, and
 * hereby grant back to Hewlett-Packard Company and its affiliated companies ("HP")
 * a non-exclusive, unrestricted, royalty-free right and license under any changes,
 * enhancements or extensions  made to the core functions of the software, including
 * but not limited to those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to HP any such changes,
 * enhancements or extensions that they make and inform HP of noteworthy uses of
 * this software.  Correspondence should be provided to HP at:
 *
 *                       Director of Intellectual Property Licensing
 *                       Office of Strategy and Technology
 *                       Hewlett-Packard Company
 *                       1501 Page Mill Road
 *                       Palo Alto, California  94304
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND HP DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL HP
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/

#include "XML_Parse.h"
#include "io.h"
#include "processor.h"
#include "time.h"
#include "xmlParser.h"
#include <iostream>

using namespace std;

void print_usage(char * argv0);
RuntimeParameter mcpat_call;

int main(int argc,char *argv[])
{
#ifdef CONFIG_STANDALONE
  char * fb[2] ;
	bool infile_specified     = false;
	int  plevel               = 5;
	extern  bool opt_for_clk;

	//cout.precision(10);
	if (argc <= 1 || argv[1] == string("-h") || argv[1] == string("--help"))
	{
		print_usage(argv[0]);
	}

  // eka, this part edited to read in more that one infile, just for test
  int j = 0;
	for (int32_t i = 0; i < argc; i++)
	{
		if (argv[i] == string("-infile"))
		{
			infile_specified = true;
			i++;
			fb[j++] = argv[ i];
      if (j > 2)
        exit(0);

		}
   //

		if (argv[i] == string("-print_level"))
		{
			i++;
			plevel = atoi(argv[i]);
		}

		if (argv[i] == string("-opt_for_clk"))
		{
			i++;
			opt_for_clk = (bool)atoi(argv[i]);
		}
	}
	if (infile_specified == false)
	{
		print_usage(argv[0]);
	}

	//parse XML-based interface
	cout<<"McPAT is computing the target processor...\n "<<endl;

  // clock codes by eka, just to see how long each method takes
  clock_t startpars, endpars;
  clock_t startproc, endproc;
  clock_t startproc2, endproc2;
  clock_t startdisp, enddisp;
  startpars = clock();
  ParseXML *p1= new ParseXML();
	p1->parse(fb[0]);
	//eka, this is for test to open another XML file
  ParseXML *p2= new ParseXML();
	p2->parse(fb[1]);
	//
  endpars = clock();
  startproc = clock();
	Processor proc(p1);
  endproc = clock();
	proc.displayEnergy(2, plevel);
  // by eka, to prepare for next calls, the way it's done now we do not need
  // to check if it is the first call or not, but the class is defined and
  // this shows the usage for future reference
  if (mcpat_call.Is_first_call()==true)
    mcpat_call.Set_no_first_call();
	//
	//
	//by eka, after the first call, next calls will call Processor2 instead of
	//allocating a new object,
  //since we are about to update the XML counters, and bypass the Parser
  //a modification  ll be required in the interface of Processor2
  //I'll patch it when we decide on how to pass the counters
  startproc2 = clock();
  proc.Processor2(p2);
	//
	//
  endproc2 = clock();
  startdisp = clock();
	proc.displayEnergy(2, plevel);
  enddisp = clock();
  cout<<endl<<"Time to call Parser: "<< (endpars - startpars);
  cout<<endl<<"Time to call Processor (1st): "<< (endproc - startproc);
  cout<<endl<<"Time to call Processor (2nd): "<< (endproc2 - startproc2);
  cout<<endl<<"Time to call displayEnergy: "<< (enddisp - startdisp) <<endl;
	delete p1;

#endif
	return 0;
}

void print_usage(char * argv0)
{
    cerr << "How to use McPAT:" << endl;
    cerr << "  mcpat -infile <input file name>  -print_level < print level for detailed output>  -opt_for_clk < 0 (optimize for EDP only)/1 (optimzed for target clock rate)>"<< endl;
    cerr << "    Note:default print level is at processor level, please increase it to see the details" << endl;
    exit(1);
}
#endif
