#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <ctime>
#include <string>
#include <stdio.h>
#include <ftw.h>
#include <fnmatch.h>
#include <algorithm>

using namespace std;

vector<string> videoFiles;

//https://stackoverflow.com/questions/983376/recursive-folder-scanning-in-c
static const char *filters[] = {
    "*.png"
};

static int fileCallback(const char *fpath, const struct stat *sb, int typeflag) {
    /* if it's a file */
    if (typeflag == FTW_F) {
        int i;
        /* for each filter, */
        for (i = 0; i < sizeof(filters) / sizeof(filters[0]); i++) {
            /* if the filename matches the filter, */
            if (fnmatch(filters[i], fpath, FNM_CASEFOLD) == 0) {
                /* do something */
                videoFiles.push_back(string(fpath));
                printf("found image: %s\n", fpath);
                break;
            }
        }
    }

    /* tell ftw to continue */
    return 0;
}




int main() {
  //grab all video files
  ftw("training_data/png", fileCallback, 16);


  return 0;
}