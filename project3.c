#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "common.h"
#include "classify.h"
/*
 * CS 361: Project 3 : create a map file from a binary cluster file.
 *
 * Authors: Tomas Castillo and Hannah Peluso
 * @version 3-1-2021
 * 
 * This file complies with the JMU honor code.
 */
int main(int argc, char *argv[])
{
    int input_fd;
    int classification_fd;
    int map_fd;
    int cluster_number;
    int bytes_read;
    unsigned char classification, cluster_type;
    char cluster_data[CLUSTER_SIZE];
    
    // We only accept running with one command line argumet: the name of the
    // data file
    if (argc != 2) {
        printf("Usage: %s data_file\n", argv[0]);
        return 1;
    }
    
    // Try opening the file for reading, exit with appropriate error message
    // if open fails
    input_fd = open(argv[1], O_RDONLY);
    if (input_fd < 0) {
        printf("Error opening file \"%s\" for reading: %s\n", argv[1], 
strerror(errno));
        return 1;
    }
    
    // Iterate through all the clusters, reading their contents 
    // into cluster_data
    cluster_number = 0;
    classification_fd = open(CLASSIFICATION_FILE, O_WRONLY | O_CREAT, 0777);
    while ((bytes_read = read(input_fd, &cluster_data, CLUSTER_SIZE)) > 0) {
        assert(bytes_read == CLUSTER_SIZE);
        classification = TYPE_UNCLASSIFIED;
        
        // Adds jpg header bit
        if (has_jpg_header(cluster_data)) classification += TYPE_JPG_HEADER;
        // Adds htm header bit
        if (has_html_header(cluster_data)) classification += TYPE_HTML_HEADER;
        // Adds jpg footer bit
        if (has_jpg_footer(cluster_data)) {
            classification += TYPE_JPG_FOOTER + TYPE_IS_JPG;
        // Adds jpg body bit 
        // To avoid false positives it wont be checked it jpg footer is already 
true
        } else if (has_jpg_body(cluster_data)) classification += TYPE_IS_JPG;
        // Adds htm footer bit
        if (has_html_footer(cluster_data)) {
            classification += TYPE_HTML_FOOTER + TYPE_IS_HTML;
        // Adds htm body bit 
        // To avoid false positives it wont be checked it htm footer is already 
true
        } else if (has_html_body(cluster_data)) classification += TYPE_IS_HTML;
        
        write(classification_fd, &classification, 1);
        cluster_number++;
    }
    
    close(input_fd);
    close(classification_fd);
    // Try opening the classification file for reading, exit with appropriate
    // error message if open fails 
    classification_fd = open(CLASSIFICATION_FILE, O_RDONLY); // Instead of opening 
this file here, you may elect to open it before the classifier loop in read/write 
mode
    if (classification_fd < 0) {
        printf("Error opening file \"%s\": %s\n", CLASSIFICATION_FILE, 
strerror(errno));
        return 1;
    }
    
    // Iterate over each cluster, reading in the cluster type attributes
    map_fd = open(MAP_FILE, O_WRONLY | O_CREAT, 0777);
    char filename[13] = "file";
    filename[12] = '\0';
    int htmFileNum = 0, jpgFileNum = 0, totalFileNum = 0;
    int currHtmCount = 0, currJpgCount = 0;
    while ((bytes_read = read(classification_fd, &cluster_type, 1)) > 0) {
        char byte = cluster_type;
        // Bit 1 mean jpg file
        if (byte & 1) {
            byte >>= 1;
            
            // Check if new jpg file is detected
            if (byte & 1) {
                totalFileNum++;
                jpgFileNum = totalFileNum;
                currJpgCount = 0;
            }
            byte >>= 1;
            // Write to file
            sprintf(&(filename[4]), "%04d", jpgFileNum);
            filename[8] = '.';
            strcpy(&(filename[9]), "jpg");
            write(map_fd, filename, 12);
            write(map_fd, &currJpgCount, 4);
            currJpgCount++;
            // If footer recent jpg file counter
            if (byte & 1) currJpgCount = 0;
        // Else means htm file
        } else {
            byte >>= 4;
            // Check if new htm file is detected
            if (byte & 1) {
                totalFileNum++;
                htmFileNum = totalFileNum;
                currHtmCount = 0;
            }
            
            byte >>= 1;
            // Write to file
            sprintf(&(filename[4]), "%04d", htmFileNum);
            filename[8] = '.';
            strcpy(&(filename[9]), "htm");
            write(map_fd, filename, 12);
            write(map_fd, &currHtmCount, 4);
            currHtmCount++;
            // If footer recent htm file counter
            if (byte & 1) currHtmCount = 0;
        }
    }
    close(map_fd);
    close(classification_fd);
    return 0;
};
