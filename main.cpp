#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <fstream>
#include <iostream>
using namespace std;

/**
 * Purpose: A struct that is used and shared between threads in order to create 
 * an interactive progress bar, depending on the data - currentStatus & TerminationValue.
 * Pass this struct and initialize the data in order for the same data to be 
 * manipulated/looked at during threads. 
 */
typedef struct {
  long * CurrentStatus; // bytes read so far 
  long InitialValue; // words 
  long TerminationValue; // maximum bytes to read 
} PROGRESS_STATUS;

/**
 * Get the size of a file.
 * @return The filesize, or 0 if the file does not exist.
 */
size_t getFileSize(const char* filename) {
  struct stat buf;
  if (stat(filename, &buf) != 0) { // indicating whether operation succeed or failed 
    return 0;
  }
  return buf.st_size;   
  // size_t: the object size in bytes (unsigned integer type)
}

/**
 * Purpose: Display the progress bar while reading the file. We first initialize the
 * struct data that is used during our wordCount call (main thread) in order to accurate
 * output the correct number of - / + onto the console to imitate a working progress bar.
 * Since the data are shared and is being looked at throughout the thread,
 * divide current (amount of chars read in so far) by the termination value
 * and multiple it by the number of +/- signs - in order to get a percentage of
 * the file read so far.
 * Based on this percentage, some logic would help output the correct
 * number of -/+ signs while ensuring which signs are to be outputted at a specific moment.  
 */
void * progress_monitor(void * prog) {
  long * current = ((PROGRESS_STATUS *) prog)->CurrentStatus; // Initializing data from struct to use in progress_monitor 
  long termination = ((PROGRESS_STATUS *) prog)->TerminationValue;
  double currentPercent = (double)*current / (double)termination; // Ensuring to cast due to decimal places when reading from extremely large files.
  double numMarkersToPrint = currentPercent * 50; // We calculate the current percentage.
  int counter = 1; // Counting the number of prints

  // This condition ensures that only 50 + / - signs are outputted and that we stop 
  // the loop once we reach the max bytes.
  while (currentPercent <= 1 && counter <= 50) {  
    currentPercent = (double)*current / (double)termination; 
    numMarkersToPrint = currentPercent * 50; // Recalculating the percentage after every iteration 

    // Ensuring that we print only the exact amount of + / - signs depending on the 
    // current percentage progress
    while (counter <= numMarkersToPrint) { 
      // Using modulo to determine if 10, 20, 30, 40, or 50 was reached.
      if (counter % 10 == 0 && counter > 0) {  
        printf("+");
      }
      else {
        printf("-");
      }
      counter++;
    }
    fflush(stdout); // outputting once the while loop finishes it's iteration 
  }
  printf("\n"); // style 

  // exiting thread because we finished printing and maximum chars were reached in the file
  pthread_exit(NULL); 
  return NULL;
}

/**
 * Purpose: wordCount takes in a file as an argument in order to open, read, and
 * act as the main thread.
 * It updates the shared data, struct. (PROGRESS_STATUS) In order for the secondary
 * thread to be able to retrieve the data to print out the corresponding +/- signs.
 * wordCount counts the number of chars, which is set to progstat->CurrentStatus,
 * and number of words.
 * Ceck for special cases such as if there is a double space to ensure we retrieve
 * an accurate word count.
 */ 
long wordCount(char* fileName) {
  ifstream inputFile;   // creating ifstream for file 
  long numOfWords = 0;  // number of words in the file
  char ch;              // current character being read
  bool lastWasSpace = true; // bool flag for special double space case 

  inputFile.open(fileName); // error checking below when we open the file. 
  if (inputFile.fail()) { 
    printf("Could not open file\n");
    return 0;
  }

  // instantiate progress status struct and its variables
  PROGRESS_STATUS *progstat = new PROGRESS_STATUS();
  long currentStatusVal = 0;
  progstat->CurrentStatus = &currentStatusVal;
  progstat->InitialValue = 0; // words counted
  progstat->TerminationValue = getFileSize(fileName); // setting number of bytes in file to TerminationValue

  // creating a thread: calling progress_monitor funciton, ensuring that the struct 
  // variable can be used as shared data, and creating the secondary thread. 
  pthread_t thread;
  pthread_create(&thread, NULL, &progress_monitor, (void *) progstat); 
  
  // ---------------------------MAIN THREAD-----------------------------------
  // count words until termination value == number of words read
  // pthread_join stops us from continuing in order to display progress bar fully
  while (inputFile.get(ch)) {
    currentStatusVal++; // increment the char count for the actual file

    if (isspace(ch)) { // checking for spaces 
      if (!lastWasSpace) { // bool for if previous char was a space 
        numOfWords++;
        lastWasSpace = true;
      }
    }
    else {
      if (ch != '\n' && ch != '\r') lastWasSpace = false; // if previous char was specifically a space, we set the bool to false to prevent the numOfWords from incrementing 
    }
  }
  // count last word if file ends with NULL after a nonspace char
  if (!lastWasSpace) ++numOfWords; 
  //-----------------------END MAIN THREAD----------------------------------------
  
  // waits for the progress monitor to finish in order to continue this function
  pthread_join(thread, NULL); 
  return numOfWords;
}

/**
 * Purpose: The main program which checks if a file exists/was passed as input
 * and calls wordCount which creates a thread separately in order to display a
 * progress-bar based on how many chars have been read in the file.
 * Then the wordcount returns the number of words read in to the main program for output.
 */
int main (int argc, char* argv[]) {
  // error checking to see if a file was passed inside.
  if (argc != 2) { 
    printf("No file specified\n");
    return 0;
  }

  long numWords = wordCount(argv[1]); // passing in filename
  cout << "There are " << numWords << " words in " << argv[1] << "." << endl; 

}
