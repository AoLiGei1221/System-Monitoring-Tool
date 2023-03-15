#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <utmp.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>

void session_users() {
    printf("### Sessions/users ###\n");
    // Open the utmp file
    setutent();
    struct utmp *user;
    // Iterate through each entry in the utmp file
    while ((user = getutent()) != NULL) {
        // Only count users who are currently logged in
        if (user -> ut_type == USER_PROCESS){
            printf(" %-9s       %s (%s)\n", user -> ut_user, user -> ut_line, user -> ut_host);
        }
    }
    // Close the utmp file
    endutent();
    printf("---------------------------------------\n");
}

void system_infomation() {
    struct utsname sys_info;
    uname(&sys_info);
    printf("### System Information ###\n");
    printf( "System Name = %s\n", sys_info.sysname);
    printf(" Machine Name = %s\n", sys_info.nodename);
    printf(" Version = %s\n", sys_info.version);
    printf(" Release = %s\n", sys_info.release);
    printf(" Architecture = %s\n", sys_info.machine);
    printf("---------------------------------------\n");
}


void memory_usage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) return;
    printf(" Memory usage: %ld kilobytes\n", usage.ru_maxrss);
}

void memory(char mem_graphics[][1024], int i_sample, int show_graphics, double *previous_used) {
    struct sysinfo s_info;
    double MemoryTotal, phy_used, SwapTotal, vir_used, difference;
    int error = sysinfo(&s_info);
    if(error) exit(1);
    MemoryTotal = s_info.totalram / (1024.0 * 1024.0 * 1024.0);
    phy_used = MemoryTotal - s_info.freeram / (1024.0 * 1024.0 * 1024.0);
    SwapTotal = MemoryTotal + s_info.totalswap / (1024.0 * 1024.0 * 1024.0);
    vir_used = phy_used + (s_info.totalswap - s_info.freeswap) / (1024.0 * 1024.0 * 1024.0);  
    if(!show_graphics){
        sprintf(mem_graphics[i_sample], "%.2f GB / %.2f GB  -- %.2f GB / %.2f GB",
            phy_used, MemoryTotal, vir_used, SwapTotal);
    }
    char sym1[128], sym2[128];
    sym1[0] = '\0';
    sym2[0] = '\0';
    char ooooo[100] = "   ";
    if(show_graphics) {
        char rest[1024];
        if(i_sample == 0){
            sprintf(rest, "%s|o 0.00 (%.2f)", ooooo, vir_used);
        }
        //positive relative change
        else if(vir_used - *previous_used > 0){
            //get the difference
            difference = vir_used - *previous_used;
            //to make the difference only two decimal and after two dec all zero
            double temp_diff = (int)((difference * 100) + 0.5) / 100.0;
            //initialize the string that wanna insert
            char temp_res[128] = {'\0'};
            //pos_flag used to indicate the diff is > 0.01 or < 0.01
            int pos_flag = 0;
            //the temp_difference > 0.01, then need to insert
            //copy the the symbol and make the flag to 1 to
            //indicate the diff is > 0.01
            //   -> to divide to two parts 1. diff is big  2. diff is small
            if(temp_diff > 0.01) {
                pos_flag = 1;
                strcpy(sym1, "#");
                strcpy(sym2, "*");
            }
            //if diff is small
            if(pos_flag == 0) {
                strcpy(sym2, "*");
                //strcpy(temp_res, sym2);
                strcat(temp_res, sym2);
            }
            //if the difference is big(>0.01)
            if(pos_flag == 1){
                for(int d = 0; d < temp_diff * 100; d+=1) {
                    //strcpy(temp_res, sym1);
                    strcat(temp_res, sym1);
                }
                strcat(temp_res, sym2);
            }
            //it is big and after for loop we need to add the ending star
            //if(pos_flag) strcat(temp_res, sym2);
            sprintf(rest, "%s|%s %.2f (%.2f)", ooooo, temp_res, difference, vir_used);
        }
        //negative relative change
        else if(vir_used - *previous_used < 0) {
            difference = vir_used - *previous_used;
            double temp_diff = (int)((fabs(difference) * 100) + 0.5) / 100.0;
            char tempppp[128] = {'\0'};
            int neg_flag = 0;
            if(fabs(difference) > 0.01) {
                neg_flag = 1;
                strcpy(sym1, ":");
                strcpy(sym2, "@");
            }
            //still have difference but it is really small
            if(difference != 0 && neg_flag == 0) {
                strcpy(sym2, "@");
                strcat(tempppp, sym2);
            }
            //difference is big
            if(neg_flag == 1){
                for(int d = 0; d < temp_diff * 100; d+=1) {
                    strcat(tempppp, sym1);
                }
                strcat(tempppp, sym2);
            }
            //it is big and after for loop we need to add the ending star
            sprintf(rest, "%s|%s %.2f (%.2f)", ooooo, tempppp, difference, vir_used);
        } else {
            //no change
            char cccc[128] = {'\0'};
            sprintf(rest, "%s|%s %.2f (%.2f)", ooooo, cccc, difference, vir_used);
        }
        sprintf(mem_graphics[i_sample], "%.2f GB / %.2f GB  -- %.2f GB / %.2f GB%s", phy_used, MemoryTotal, vir_used, SwapTotal, rest);
        *previous_used = vir_used;
    }
    
}

int get_num_cpu() {
    FILE *f;
    f = fopen("/proc/stat", "r");
    char buff[1024];
    int num = 0;
    while(fscanf(f, "%s", buff) != EOF) {
        if(strstr(buff, "cpu") != NULL) {
            num++;
        }
    }
    num = num - 1;
    fclose(f);
    return num;
}

void cpu(char cpu_w_graphics[][1024], int i_sample, long *previous_idle, long *previous_total, int g, char cpu_g[][1024]) {
    /* get total cpu usage */
    long int usage, idle, total = 0;
    int index = 1, idx_str = 0;
    double total_usage;
    FILE *fp;
    char buff[1024], *temp, ch;
    /*
    f = fopen("/proc/stat", "r");
    fgets(buff, 1024, f); // read the first line to buff
    fclose(f);
    */
    fp = fopen("/proc/stat", "r");
    if(fp == NULL) exit(1);
    while((ch = fgetc(fp)) != EOF){
        if(ch == '\n') {buff[idx_str] = '\0'; break;}
        if (isdigit(ch) != 0) buff[idx_str++] = ch;
        else if(ch == 32) buff[idx_str++] = ch;
        else continue;
    }
    fclose(fp);
    temp = strtok(buff, " ");
    while (temp != NULL) {
        usage = strtol(temp, NULL, 10);
        total = total + usage;
        if (index++ == 4) {
            idle = usage;
        }
        temp = strtok(NULL, " ");
    }
    total_usage = 100.0 - ((double) (idle - *previous_idle) * 100.0/ (double) (total - *previous_total));
    //total_usage = (((double)(total - *previous_total) - (double)(idle - *previous_idle)) / (double)(total - *previous_total)) * 100;
    sprintf(cpu_w_graphics[i_sample], " total cpu use = %.2lf%%", total_usage);
    *previous_idle = idle;
    *previous_total = total;
    if(g) {
        char iiiii[128] = "         ";
        if(i_sample == 0){
            sprintf(cpu_g[i_sample], "%s%s %.2lf", iiiii, "|||", total_usage);
        }else{
            char bar[1024];
            bar[0] = '\0';
            int num = total_usage / 0.77, count = 0;
            while(count < num) {bar[count++] = '|';}
            bar[count] = '\0';
            sprintf(cpu_g[i_sample], "%s%s %.2lf", iiiii, bar, total_usage);
        }
    }
}

//**************************************************************************
void display_user_f(int flag, int system) {
    if(flag){
        if(system){
            printf("---------------------------------------\n");
            session_users();
        }else{
            session_users();
        }
    }
}

void check_seq(int seq, int iter) {
    if (seq) {
        printf(">>> Iteration %d\n", iter);
        memory_usage();
        printf("---------------------------------------\n");
    } else {
        printf("\x1b[H\x1b[2J");
    }
}
void display_given_val(int seq, int num, int dly) {
    if(!seq) {
        printf("Nbr of samples: %d -- every %d secs\n", num, dly);
        memory_usage();
        printf("---------------------------------------\n");
    }
}
//**************************************************************************

int main(int argc, char **argv) {
    // defualt arguments
    int n = 10, delay = 1, j = 0, first_occur = 1, show_graphics = 0, show_only_system = 0, show_only_user = 0, sequential = 0;
    // process arguments
    for (int i = 1; i < argc; i++) {
        if (strstr(argv[i], "--samples=") != NULL){
            n = strtol(argv[i] + 10, NULL, 10);
            continue;
        }
        else if (strstr(argv[i], "--tdelay=") != NULL){
            delay = strtol(argv[i] + 9, NULL, 10);
            continue;
        }
        else if (strcmp(argv[i], "--sequential") == 0){
            sequential = 1;
            continue;
        }
        else if (strcmp(argv[i], "--graphics") == 0){
            show_graphics = 1;
            continue;
        }
        else if (strcmp(argv[i], "-g") == 0) {
            show_graphics = 1;
            continue;
        }
        else if (strcmp(argv[i], "--user") == 0){
            show_only_user = 1;
            continue;
        }
        else if (strcmp(argv[i], "--system") == 0){
            show_only_system = 1;
            continue;
        }

        while(j < strlen(argv[i])){
            if(isdigit(argv[i][j++]) == 0){
                continue;
            }
        }
        if(first_occur){
            n = atoi(argv[i]);
            first_occur = 0;
        }else{
            delay = atoi(argv[i]);
        }
    }
    
    char cpu_without_g[n][1024];
    char cpu_graphics[n][1024];
    char mem_graphics[n][1024];
    int all_zero = 0;
    while(all_zero < n){
      cpu_graphics[all_zero][0] = '\0';
      mem_graphics[all_zero][0] = '\0';
      cpu_without_g[all_zero++][0] = '\0';
    }

    long int previous_idle = 0;
    long int previous_total = 0;
    double previous_used = 0.0;

    for(int i = 0; i < n; i++) {
        //just ./a.out
        if(!show_only_system && !show_only_user) {
            check_seq(sequential, i);
            memory(mem_graphics, i, show_graphics, &previous_used);
            display_given_val(sequential, n, delay);
            //--system
            int k = 0;
            if(!show_only_system) {
                printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
                //--sequential
                if(sequential) {
                    while(k < n) {
                        if(k != i){
                            printf("    \n");
                            k++;
                        }else{
                            printf("%s\n", mem_graphics[k++]);
                        }
                    }
                } 
                else {
                    int f = 0;
                    while(f < n)
                        printf("%s\n", mem_graphics[f++]); 
                }
            }
            //--users
            display_user_f(!show_only_user, !show_only_system);
            //--system
            //cpu and num of cores part
            if(show_graphics == 0) {
                cpu(cpu_without_g, i, &previous_idle, &previous_total, show_graphics, cpu_graphics);
                if(!show_only_system){
                    printf("Number of cores: %d\n", get_num_cpu());
                    for(int d = 0; d < n; d++){
                        printf("%s\n", cpu_without_g[d]);
                        if(d != n - 1){
                            printf("\033[1A");
                        }
                    }
                }
            } else {
                cpu(cpu_without_g, i, &previous_idle, &previous_total, show_graphics, cpu_graphics);
                printf("Number of cores: %d\n", get_num_cpu());
                printf("%s\n", cpu_without_g[i]);
                int a = 0;
                while(a < n) {
                    if(!sequential) {
                        printf("%s\n", cpu_graphics[a++]);
                    } else {
                        if(a != i){
                            printf("    \n");
                            a++;
                        }else{
                            printf("%s\n", cpu_graphics[a++]);
                        }
                    }
                }
            }
            sleep(delay);
        } else {
            check_seq(sequential, i);
            memory(mem_graphics, i, show_graphics, &previous_used);
            display_given_val(sequential, n, delay);
            //--system
            int k = 0;
            if(show_only_system) {
                printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
                //--sequential
                if(sequential) {
                    while(k < n) {
                        if(k != i){
                            printf("    \n");
                            k++;
                        }else{
                            printf("%s\n", mem_graphics[k++]);
                        }
                    }
                } else {
                    int f = 0;
                    while(f < n) 
                    printf("%s\n", mem_graphics[f++]); 
                }
            }
            //--users
            display_user_f(show_only_user, show_only_system);
            //--system
            //cpu and num of cores part
            if(show_graphics == 0) {
                cpu(cpu_without_g, i, &previous_idle, &previous_total, show_graphics, cpu_graphics);
                if(show_only_system){
                    printf("Number of cores: %d\n", get_num_cpu());
                    for(int d = 0; d < n; d++){
                        printf("%s\n", cpu_without_g[d]);
                        if(d != n - 1){
                            printf("\033[1A");
                        }
                    }
                }
            } else {
                cpu(cpu_without_g, i, &previous_idle, &previous_total, show_graphics, cpu_graphics);
                printf("Number of cores: %d\n", get_num_cpu());
                printf("%s\n", cpu_without_g[i]);
                int b = 0;
                while(b < n){
                    if(!sequential) {
                        printf("%s\n", cpu_graphics[b++]);
                    } else {
                        if(b != i){
                            printf("    \n");
                            b++;
                        }else{
                            printf("%s\n", cpu_graphics[b++]);
                        }
                    }
                }
            }
            sleep(delay);
        }
    }
    printf("---------------------------------------\n");
    system_infomation();
    return 0;
}