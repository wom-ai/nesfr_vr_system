/*
 * references:
 *   - https://stackoverflow.com/questions/42778470/is-it-possible-to-use-nmap-functionalities-in-c
 *   - https://stackoverflow.com/questions/50927192/nmap-to-scan-mac-address-for-remote-machine-by-non-root-user
 */

#include <cstdio>

int main(int argc, char const *argv[]) {
    //FILE *pin = popen("nmap -sn 192.168.0.0/24","r");
    FILE *pin = popen("arp -n","r");
    if ( pin ) {
        while (!feof(pin)) {
            char *line = nullptr;
            size_t len = 0;
            ssize_t read = getline(&line, &len, pin);
            printf("%s", line);
        }
        pclose(pin);
    } 
}
