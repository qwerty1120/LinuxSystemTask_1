
#include "ssu_header.h"

void Init();
void treemake(){//list 트리구조를 위한 디렉터리 만드는 함수
    timeList *head = (timeList*)malloc(sizeof(timeList));
    timeList *curr = (timeList*)malloc(sizeof(timeList));
    char *buf = (char *)malloc(sizeof(char)*PATHMAX);
    char *Newbuf = (char *)malloc(sizeof(char)*PATHMAX);
    int fd;
    head=backuplist;
    curr=head->next;
    while(curr->next!=NULL){
        curr=curr->next;
        sprintf(buf, "%s/tree%s", backupPATH, curr->path+strlen(homePATH));
        strcpy(Newbuf, buf);
        for(int j=0;buf[j]!=0;j++){// 디렉터리가 없으면 생성
            if(buf[j]=='/') {
                Newbuf[j] = 0;
                if(access(Newbuf,F_OK)){
                    mkdir(Newbuf,0777);
                }
                Newbuf[j]='/';
            }
        }
        if((fd=open(buf,O_RDONLY|O_CREAT))<0){
            fprintf(stderr, "List Error\n");
        }
    }
}
void list_tree(int height, char *isLastDir) {//list명령어에서 tree 출력하기
    char *treePATH = (char*)malloc(sizeof(char)*PATHMAX);
    char *treebuf = (char*)malloc(sizeof(char)*PATHMAX);
    struct dirent **namelist;
    int i, count, lastIdx, firstidx = 0;
    struct stat statbuf;

    getcwd(treePATH, PATHMAX);//현재 디렉터리 받아오기
    sprintf(treebuf, "%s", treePATH+strlen(backupPATH)+5);//tree 경로 빼기

    sprintf(treePATH, "/home%s", treebuf);//tree 경로 빼기
    if ((count = scandir(".", &namelist, NULL, alphasort)) == -1) {
        return;
    }
    for (i = count - 1; i >= 0; i--) {//디렉터리 구성요소 체크
        if (!strcmp(".", namelist[i]->d_name) || !strcmp("..", namelist[i]->d_name)) {
            firstidx = i;
            continue;
        }

        lastIdx = i;
        break;
    }
    for (i = 0; i < count; i++) {
        if (!strcmp(".", namelist[i]->d_name) || !strcmp("..", namelist[i]->d_name)) {//현재나 위쪽 디렉터리 넘기기
            free(namelist[i]);
            continue;
        }
        if (stat(namelist[i]->d_name, &statbuf) < 0) {
            free(namelist[i]);
            continue;
        }
        for (int i = 0; i < height; i++) {
            if (isLastDir[i] == 0)   //마지막 아니면 잇기
                printf("│");
            else {
                if (!i) { printf("%3d.", ++treecnt); }
                printf(" ");
            }
            printf("   ");
        }
        if (i != lastIdx) {
            printf("├─ %s\n", namelist[i]->d_name); //밑에 자식 잇기
            isLastDir[height] = 0;
        } else {
            printf("└─ %s\n", namelist[i]->d_name);
            isLastDir[height] = 1;
        }
        sprintf(treelist[treelistcnt++], "%s/%s", treePATH, namelist[i]->d_name);
        if (S_ISDIR(statbuf.st_mode)) {   //디렉토리라면
            chdir(namelist[i]->d_name); //작업디렉토리 이동
            list_tree(height + 1, isLastDir);  //깊이 1 증가
            chdir("..");  //돌아오기
        }
        free(namelist[i]);
    }
    free(namelist);
    if (height == 1)printf(">> ");// 루트 디렉터리인 경우 트리 끝을 표시
}

timeList *Gettime_list() {//경로 받기 log 보고 해당 경로와 관련있는 backup 속 dir 찾기
    struct stat statbuf;
    timeList *head = (timeList *) malloc(sizeof(timeList));
    timeList *curr = (timeList *) malloc(sizeof(timeList));
    head->next = curr;
    head->prev = NULL;
    char *buf = (char *) malloc(sizeof(char *) * STRMAX);
    char *filepath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *number = (char *) malloc(sizeof(char *) * NAMEMAX);
    char *listpath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *backpath = (char *) malloc(sizeof(char *) * PATHMAX);
    int fd;
    int len;
    int rech = 0;//remove, recover 체크용
    int numch = 1, num = 0, numpath = 0, path = 0;

    if (stat(ssubak, &statbuf) < 0) {
        fprintf(stderr, "stat error for ssubak.log\n");
        exit(1);
    }

    if ((fd = open(ssubak, O_RDONLY)) < 0) {// ssubak.log 파일 열기
        fprintf(stderr, "ERROR: open error for %s\n", filepath);
        exit(1);
    }

    if ((len = read(fd, buf, statbuf.st_size)) < 0) {
        fprintf(stderr, "read error for ssubak.log\n");
        exit(1);
    }// ssubak.log 파일 내용 분석 및 리스트 구성
    for (int i = 0; i < statbuf.st_size; i++) {
        if (numch && buf[i] != ' ') {
            number[num++] = buf[i];
        } else if (numch) {
            number[num] = numch = 0;
            num = 0;
        } else if (numpath && buf[i] != '\"')
            listpath[path++] = buf[i];
        else if (!numpath && buf[i] == '\"')
            numpath = 1;
        else if (numpath && buf[i] == '\"') {
            listpath[path] = numpath = 0;
            path = 0;
            i++;
            if (buf[i + 1] == 'r')rech = 1;//backup인지 remove인지 체크

            while (buf[i++] != '\"');
            while (buf[i] != '\"') {
                backpath[path++] = buf[i++];
            }
            backpath[path] = 0;
            path = 0;
            while (buf[i++] != '\n');
            numch = 1;
            i--;
            // 백업된 디렉터리를 리스트에 추가
            if (!rech) {
                timeList *new = (timeList *) malloc(sizeof(timeList));
                new->next = NULL;
                strcpy(new->dirtime, number);
                strcpy(new->path, listpath);
                strcpy(new->backuppath, backpath);
                new->prev = curr;
                curr->next = new;
                curr = curr->next;
                curr->next = NULL;
            } else {
                curr = head;// remove, recover인 경우 해당 백업 디렉터리 제거
                while (1) {
                    curr = curr->next;
                    if (!strcmp(curr->dirtime, number) && !strcmp(curr->backuppath, listpath)) {
                        if (curr->prev == NULL && curr->next == NULL) {
                            curr = head;
                            curr->next = NULL;
                        }
                        else if (curr->prev == NULL) {
                            head->next = curr->next;
                            curr = head;
                        }
                        else if (curr->next == NULL) {
                            curr = curr->prev;
                            curr->next = NULL;
                        }
                        else {
                            curr->next->prev = curr->prev;
                            curr->prev->next = curr->next;
                        }
                        rech = 0;
                        break;
                    }
                    if (curr->next == NULL)break;
                }
                curr = head;
                while (curr->next != NULL) {
                    curr = curr->next;
                }
            }
        }
    }
    return head;
}

int RecoverFile(char *path, char *newPath, int commandopt) {
    fileNode *head = (fileNode *) malloc(sizeof(fileNode));
    fileNode *curr = head;
    timeList *logcurr = backuplist;
    struct dirent **namelist;
    struct stat tmpbuf;
    char *tmpPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *buf = (char *) malloc(sizeof(char *) * PATHMAX);
    char *filepath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *filename = (char *) malloc(sizeof(char *) * NAMEMAX);
    char *input = (char *) malloc(sizeof(char *) * STRMAX);
    char *date = (char *) malloc(sizeof(char *) * STRMAX);
    char *logpath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *file_backuppath = (char *) malloc(sizeof(char *) * STRMAX);
    char *fileswp = (char *)malloc(sizeof(char*)*STRMAX);
    int fd1, fd2;
    char *Newbuf = (char *) malloc(sizeof(char *) * STRMAX);
    int i;
    int idx=0;
    int cnt;
    int num;
    int len;
    int log_fd;
    char ch=0;

    strcpy(Newbuf, newPath);
    for(int j=0;newPath[j]!=0;j++){// 디렉터리가 없으면 생성
        if(newPath[j]=='/') {
            Newbuf[j] = 0;
            if(access(Newbuf,F_OK)){
                mkdir(Newbuf,0777);
            }
            Newbuf[j]='/';
        }
    }

    sprintf(filepath, "%s%s", backupPATH, path + strlen(homePATH));

    for (idx = strlen(filepath) - 1; filepath[idx] != '/'; idx--);
    strcpy(filename, filepath + idx + 1);
    filepath[idx] = '\0';
    strcpy(fileswp,filepath);
    if (logcurr->next != NULL) {// 로그 리스트를 확인하여 백업된 파일의 정보를 파일 노드에 추가
        while (1) {
            logcurr = logcurr->next;
            if (!strcmp(logcurr->path, path)) {
                strcpy(tmpPath, logcurr->backuppath);
                tmpPath[strlen(logcurr->backuppath)] = 0;

                if (lstat(tmpPath, &tmpbuf) < 0) {
                    continue;
                }
                fileNode *new = (fileNode *) malloc(sizeof(fileNode));
                strcpy(new->path, logcurr->backuppath);
                new->statbuf = tmpbuf;
                curr->next = new;
                curr = curr->next;
            }
            if (logcurr->next == NULL)break;
        }
    } else {
        printf("backup dir is empty.");
        return 0;
    }
    if(commandopt & OPT_L){// OPT_L 옵션이 설정된 경우 가장 최근 백업 파일 선택
        curr=head;
        if (head->next == NULL) {
            printf("no backup file(s) of \"%s\"\n", path);
            return 1;
        }
        while(curr->next->next!=NULL){
            curr=curr->next;
        }head=curr;
    }
    if (head->next == NULL) {
        printf("no backup file(s) of \"%s\"\n", path);
        return 1;
    } else if (head->next->next == NULL) { // 백업 파일이 하나만 있는 경우
        if (access(newPath, F_OK) != -1 && !cmpHash(newPath, head->next->path)) {
            printf("\"%s\" is not changed with \"%s\"\n", head->next->path, newPath);
            return 1;
        }

        if ((fd1 = open(head->next->path, O_RDONLY)) < 0) {
            fprintf(stderr, "ERROR: open error for %s\n", head->next->path);
            return 1;
        }

        if ((fd2 = open(newPath, O_CREAT | O_TRUNC | O_WRONLY, 777)) < 0) {
            fprintf(stderr, "ERROR: open error for %s\n", newPath);
            return 1;
        }

        while ((len = read(fd1, buf, head->next->statbuf.st_size)) > 0) {//내용 복사
            write(fd2, buf, len);
        }strcpy(filepath, fileswp);//filepath가 fd로 인해 내용변하는 오류가 있어 swap해줌

        if (remove(head->next->path)) {
            fprintf(stderr, "ERROR: remove error for %s", head->next->path);
        }

        strcpy(date, head->next->path + strlen(backupPATH)+1);//date 추출
        for(int k=0;k<strlen(date);k++){
            if(date[k]=='/') {date[k]=0;break;}
        }

        printf("\"%s\" recovered to \"%s\"\n", head->next->path, newPath);
        sprintf(logpath, "%s : \"%s\" recovered to \"%s\"\n", date, head->next->path, newPath);
        len = strlen(date) + strlen(head->next->path) + strlen(newPath) + 22;//log에 쓸 내용
        logpath[len] = 0;//todo : dir recover == log message weird;//해결함

        if ((log_fd = open(ssubak, O_WRONLY | O_APPEND)) < 0) {//이어서 쓸 수 있게
            fprintf(stderr, "ERROR: open error for %s\n",ssubak);
            return 1;
        }
        write(log_fd, logpath, len);//작성
        close(log_fd);

        strcpy(file_backuppath, head->next->path);
        file_backuppath[strlen(head->next->path) - strlen(filename) - 1] = 0;//dir 이름

        if ((cnt = scandir(file_backuppath, &namelist, NULL, alphasort)) == -1) {
            fprintf(stderr, "ERROR: scandir error for %s\n", filepath);
            return 1;
        }
        if (cnt < 3) remove(file_backuppath);

        free(head);
    } else {// 백업 파일이 여러 개 있는 경우
        printf("backup files of \"%s\"\n", path);
        printf("0. exit\n");
        curr = head->next;
        for (i = 1; curr != NULL; curr = curr->next) {
            strcpy(date, curr->path + (strlen(backupPATH))+1);
            date[12]=0;
            printf("%d. %s\t\t%sbytes\n", i, date, cvtNumComma(curr->statbuf.st_size));
            i++;
        }
        printf("Choose file to recover\n");

        while (true) {
            printf(">> ");
            fgets(input, sizeof(input), stdin);
            input[strlen(input) - 1] = '\0';

            num = atoi(input);
            if (num < 0 || num >= i) {
                printf("wrong input!\n");
                continue;
            }

            if (num == 0) return 1;

            curr = head->next;
            for (i = 1; curr != NULL; curr = curr->next) {
                if (i == num) {
                    if (access(newPath, F_OK) != -1 && !cmpHash(newPath, curr->path)) {//내용 변한 정도 체크
                        printf("\"%s\" is not changed with \"%s\"\n", curr->path, newPath);
                        return 1;
                    }

                    if ((fd1 = open(curr->path, O_RDONLY)) < 0) {
                        fprintf(stderr, "ERROR: open error for %s\n", curr->path);
                        return 1;
                    }

                    if ((fd2 = open(newPath, O_CREAT | O_TRUNC | O_WRONLY, 777)) < 0) {
                        fprintf(stderr, "ERROR: open error for %s\n", newPath);
                        return 1;
                    }

                    while ((len = read(fd1, buf, curr->statbuf.st_size)) > 0) {//내용 복사
                        write(fd2, buf, len);
                    }

                    if (remove(curr->path)) {
                        fprintf(stderr, "ERROR: remove error for %s", curr->path);
                    }

                    strcpy(date, curr->path + strlen(backupPATH)+1);//시간 추출
                    for(int k=0;k<strlen(date);k++){
                        if(date[k]=='/') {date[k]=0;break;}
                    }

                    printf("\"%s\" recovered to \"%s\"\n", curr->path, newPath);
                    sprintf(logpath, "%s : \"%s\" recovered to \"%s\"\n", date, curr->path, newPath);//로그 내용
                    len = strlen(date) + strlen(curr->path) + strlen(newPath) + 22;
                    logpath[len] = 0;

                    if ((log_fd = open(ssubak, O_WRONLY | O_APPEND)) < 0) {//이어서 쓸 수 있게
                        fprintf(stderr, "ERROR: open error for %s\n",ssubak);
                        return 1;
                    }
                    write(log_fd, logpath, len);
                    close(log_fd);

                    strcpy(file_backuppath, curr->path);
                    file_backuppath[strlen(curr->path) - strlen(filename) - 1] = 0;//dir 이름

                    if ((cnt = scandir(file_backuppath, &namelist, NULL, alphasort)) == -1) {
                        fprintf(stderr, "ERROR: scandir error for %s\n", filepath);
                        return 1;
                    }
                    if (cnt < 3) remove(file_backuppath);
                    free(curr);
                    break;
                }
                i++;
            }
            break;
        }
    }
    return 0;
}
int RecoverDir(char *path, char *newPath, int command_opt) {
    struct stat statbuf;
    struct stat buf;
    int cnt;
    struct dirent **namelist;
    char *tnpPath = (char *) malloc(sizeof(char) * PATHMAX);
    char *tmpPath = (char *) malloc(sizeof(char) * PATHMAX);
    char *Newbuf = (char *) malloc(sizeof(char) * PATHMAX);
    int i;

    strcpy(Newbuf, newPath);
    for(int j=0;newPath[j]!=0;j++){
        if(newPath[j]=='/') {
            Newbuf[j] = 0;
            if(access(Newbuf,F_OK)){
                mkdir(Newbuf,0777);
            }
            Newbuf[j]='/';
        }
    }
    if(command_opt&OPT_N)// OPT_N 옵션이 설정되어 있고 새로운 경로가 없으면 끝까지 생성
        if (access(newPath, F_OK))
            mkdir(newPath, 0777);
    if (lstat(path, &statbuf) < 0) {
        fprintf(stderr, "Usage : remove <FILENAME> [OPTION]\n");
        return 1;
    }

    if (S_ISDIR(statbuf.st_mode)) {
        if ((cnt = scandir(path, &namelist, NULL, alphasort)) == -1) {
            fprintf(stderr, "ERROR: scandir error for %s\n", path);
            return 1;
        }

        for (i = 0; i < cnt; i++) {// 디렉터리 내의 모든 파일 및 디렉터리에 대해 재귀적으로 복구 수행
            if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;

            sprintf(tmpPath, "%s/%s", path, namelist[i]->d_name);
            sprintf(tnpPath, "%s/%s", newPath, namelist[i]->d_name);
            if(command_opt & (OPT_R | OPT_D))// 재귀적으로 디렉터리 복구 수행
                RecoverDir(tmpPath, tnpPath, command_opt & (OPT_R|OPT_L));//D는 한번 시행 후 삭제
        }
    } else {
        //printf("%s", newPath);
        RecoverFile(path, newPath, command_opt);
    }
    return 0;
}

int RecoverCommand(command_parameter *parameter) {
    struct stat statbuf;
    char *tmpPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *originPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *backupPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *newPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char **newPathList = NULL;
    int newPathDepth = 0;
    int i;

    strcpy(originPath, parameter->filename);
    sprintf(backupPath, "%s%s", backupPATH, originPath + strlen(homePATH));//==filepath

    if (lstat(originPath, &statbuf) < 0) {
        fprintf(stderr, "ERROR: lstat error for %s\n", originPath);
        return 1;
    }
    if (!S_ISREG(statbuf.st_mode) && !S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "ERROR: %s is not directory or regular file\n", originPath);
        return -1;
    }
    // 디렉터리 옵션이 없는 경우 오류 처리
    if (S_ISDIR(statbuf.st_mode) && !(parameter->commandopt & OPT_R) && !(parameter->commandopt & OPT_D)) {
        fprintf(stderr, "ERROR: %s is a directory\n - use \'-r, -d\' option or input in file path.\n", originPath);
        return -1;
    }

    if (parameter->commandopt & OPT_N) {
        strcpy(newPath, parameter->tmpname);
    } else {
        strcpy(newPath, originPath);
    }
    strcpy(tmpPath, newPath);
    if ((newPathList = GetSubstring(tmpPath, &newPathDepth, "/")) == NULL) {
        fprintf(stderr, "ERROR: %s can't be recovered\n", newPath);
        return -1;
    }

    strcpy(tmpPath, "");
    for (i = 0; i < newPathDepth - 1; i++) {
        strcat(tmpPath, "/");
        strcat(tmpPath, newPathList[i]);

        if (access(tmpPath, F_OK))
            mkdir(tmpPath, 0777);
    }

    if (parameter->commandopt & (OPT_R|OPT_D)) {
        if (S_ISREG(statbuf.st_mode)) {
            fprintf(stderr, "ERROR: %s is not directory.\n", originPath);
            return -1;
        }// 메인 디렉터리 리스트 초기화
        mainDirList = (dirList *) malloc(sizeof(dirList));
        dirNode *head = (dirNode *) malloc(sizeof(dirNode));
        mainDirList->head = head;
        dirNode *curr = head->next;
        dirNode *new = (dirNode *) malloc(sizeof(dirNode));
        strcpy(new->path, originPath);
        strcpy(new->backupPath, backupPath);
        strcpy(new->newPath, newPath);
        curr = new;
        mainDirList->tail = curr;

        while (curr != NULL) {// 각 디렉터리에 대해 복구 수행
            RecoverDir(curr->path, curr->newPath, parameter->commandopt);
            curr = curr->next;
        }
    } else {
        RecoverFile(originPath, newPath, parameter->commandopt);
    }
}

int RemoveFile(char *path, int commandopt) {
    fileNode *head = (fileNode *) malloc(sizeof(fileNode));
    fileNode *curr = head;
    timeList *logcurr = backuplist;
    struct stat statbuf, tmpbuf;
    char *originPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *filepath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *logpath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *file_backuppath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *filename = (char *) malloc(sizeof(char *) * PATHMAX);
    char *tmpPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *date = (char *) malloc(sizeof(char *) * PATHMAX);
    struct dirent **namelist;
    int cnt;
    int idx;
    int i;
    int log_fd;
    int len;
    char input[STRMAX];
    int num;

    strcpy(originPath, path);
    sprintf(filepath, "%s%s", backupPATH, path + strlen(homePATH));

    for (idx = strlen(filepath) - 1; filepath[idx] != '/'; idx--);
    strcpy(filename, filepath + idx + 1);
    filepath[idx] = '\0';

    if (logcurr->next != NULL) {// 백업 로그에서 해당 파일을 찾아 파일 노드에 추가
        while (1) {
            logcurr = logcurr->next;
            if (!strcmp(logcurr->path, originPath)) {
                strcpy(tmpPath, logcurr->backuppath);
                tmpPath[strlen(logcurr->backuppath)] = 0;

                if (lstat(tmpPath, &tmpbuf) < 0) {
                    continue;
                }
                fileNode *new = (fileNode *) malloc(sizeof(fileNode));
                strcpy(new->path, logcurr->backuppath);
                new->statbuf = tmpbuf;
                curr->next = new;
                curr = curr->next;
            }
            if (logcurr->next == NULL)break;
        }
    } else {
        printf("backup dir is empty.");
        return 0;
    }

    curr = head->next;
    if(commandopt & OPT_A){
        while(curr!=NULL){// 모든 백업 파일을 삭제하고 로그를 기록
            strcpy(date, curr->path + (strlen(backupPATH)) + 1);
            for(int k=0;k<strlen(date);k++){
                if(date[k]=='/') {date[k]=0;break;}
            }

            strcpy(file_backuppath, curr->path);
            file_backuppath[strlen(curr->path) - strlen(filename) - 1] = 0;//dir 이름

            printf("\"%s\" removed by \"%s\"\n", curr->path, originPath);
            remove(curr->path);

            if ((cnt = scandir(file_backuppath, &namelist, NULL, alphasort)) == -1) {
                fprintf(stderr, "ERROR: scandir error for %s\n", filepath);
                return 1;
            }
            if (cnt < 3) remove(file_backuppath);

            sprintf(logpath, "%s : \"%s\" removed by \"%s\"\n", date, curr->path, originPath);
            len = strlen(date) + strlen(curr->path) + strlen(originPath) + 20;
            logpath[len] = 0;

            if ((log_fd = open(ssubak, O_WRONLY | O_APPEND)) < 0) {//이어서 쓸 수 있게
                fprintf(stderr, "ERROR: open error for %s\n", ssubak);
                return 1;
            }
            write(log_fd, logpath, len);
            close(log_fd);
            curr=curr->next;
        }
        return 1;
    }

    if (head->next == NULL) {
        printf("no backup file(s) of \"%s\"\n", originPath);
        return 1;
    } else if (head->next->next == NULL) {// 백업 파일 삭제 및 로그 기록

        strcpy(file_backuppath, head->next->path);
        file_backuppath[strlen(head->next->path) - strlen(filename) - 1] = 0;//dir 이름

        if (remove(head->next->path)) {
            fprintf(stderr, "ERROR: remove error for %s", head->next->path);
        }

        if ((cnt = scandir(file_backuppath, &namelist, NULL, alphasort)) == -1) {
            fprintf(stderr, "ERROR: scandir error for %s\n", filepath);
            return 1;
        }
        if (cnt < 3) remove(file_backuppath);
        strcpy(date, head->next->path + (strlen(backupPATH)) + 1);
        for(int k=0;k<strlen(date);k++){
            if(date[k]=='/') {date[k]=0;break;}
        }
        printf("\"%s\" removed by \"%s\"\n", head->next->path, originPath);

        sprintf(logpath, "%s : \"%s\" removed by \"%s\"\n", date, head->next->path, originPath);
        len = strlen(date) + strlen(head->next->path) + strlen(originPath) + 20;
        logpath[len] = 0;

        if ((log_fd = open(ssubak, O_WRONLY | O_APPEND)) < 0) {//이어서 쓸 수 있게
            fprintf(stderr, "ERROR: open error for %s\n",ssubak);
            return 1;
        }
        write(log_fd, logpath, len);
        close(log_fd);
        free(head);
    } else {// 사용자가 삭제할 백업 파일을 선택하고 삭제 후 로그 기록
        printf("backup files of \"%s\"\n", originPath);
        printf("0. exit\n");
        curr = head->next;
        for (i = 1; curr != NULL; curr = curr->next) {
            strcpy(date, curr->path + (strlen(backupPATH)) + 1);
            date[strlen(date) - strlen(filename) - 1] = 0;
            printf("%d. %s\t%sbytes\n", i, date, cvtNumComma(curr->statbuf.st_size));
            i++;
        }
        printf("Choose file to remove\n");

        while (true) {
            printf(">> ");
            fgets(input, sizeof(input), stdin);
            input[strlen(input) - 1] = '\0';

            num = atoi(input);
            if (num < 0 || num >= i) {
                printf("wrong input!\n");
                continue;
            }
            if (num == 0) return 1;

            curr = head->next;
            for (i = 1; curr != NULL; curr = curr->next) {
                if (i == num) {
                    strcpy(file_backuppath, curr->path);
                    file_backuppath[strlen(curr->path) - strlen(filename) - 1] = 0;

                    if (remove(curr->path)) {
                        fprintf(stderr, "ERROR: remove error for %s", curr->path);
                    }
                    if ((cnt = scandir(file_backuppath, &namelist, NULL, alphasort)) == -1) {
                        fprintf(stderr, "ERROR: scandir error for %s\n", filepath);
                        return 1;
                    }
                    if (cnt < 3) remove(file_backuppath);
                    strcpy(date, curr->path + (strlen(backupPATH)) + 1);//시간데이터 추출
                    for(int k=0;k<strlen(date);k++){
                        if(date[k]=='/') {date[k]=0;break;}
                    }

                    printf("\"%s\" removed by \"%s\"\n", curr->path, originPath);

                    sprintf(logpath, "%s : \"%s\" removed by \"%s\"\n", date, curr->path, originPath);
                    len = strlen(date) + strlen(curr->path) + strlen(originPath) + 20;
                    logpath[len] = 0;

                    if ((log_fd = open(ssubak, O_WRONLY | O_APPEND)) < 0) {//이어서 쓸 수 있게
                        fprintf(stderr, "ERROR: open error for %s\n", ssubak);
                        return 1;
                    }
                    write(log_fd, logpath, len);
                    close(log_fd);
                    free(curr);
                    break;
                }
                i++;
            }
            break;
        }
    }
    return 0;
}

int RemoveAll(char *path, int flag, int command_opt) {
    struct stat statbuf;
    struct stat buf;
    int cnt;
    struct dirent **namelist;
    char *tmpPath = (char *) malloc(sizeof(char) * PATHMAX);
    int i;

    if (lstat(path, &statbuf) < 0) {
        fprintf(stderr, "Usage : remove <FILENAME> [OPTION]\n");
        return 1;
    }

    if (S_ISDIR(statbuf.st_mode)) {
        if ((cnt = scandir(path, &namelist, NULL, alphasort)) == -1) {
            fprintf(stderr, "ERROR: scandir error for %s\n", path);
            return 1;
        }

        for (i = 0; i < cnt; i++) {// 현재 디렉터리나 상위 디렉터리인 경우 넘어감
            if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;

            sprintf(tmpPath, "%s/%s", path, namelist[i]->d_name);

            if(command_opt & (OPT_R | OPT_D))
                RemoveAll(tmpPath, flag, command_opt & (OPT_R | OPT_A));
        }
    } else {// 파일인 경우 RemoveFile 함수 호출
        RemoveFile(path, command_opt);
    }
    return 0;
}
int RemoveDirch(char *path){
    struct dirent **namelist;
    struct stat statbuf;
    char *tmpPath = (char *) malloc(sizeof(char) * PATHMAX);
    int cnt,i;

    if (lstat(path, &statbuf) < 0) {
        return 1;
    }
    if (S_ISDIR(statbuf.st_mode)) {
        if ((cnt = scandir(path, &namelist, NULL, alphasort)) == -1) {
            fprintf(stderr, "ERROR: scandir error for %s\n", path);
            return 1;
        }
        for (i = 0; i < cnt; i++) {// 현재 디렉터리나 상위 디렉터리인 경우 넘어감
            if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
            sprintf(tmpPath, "%s/%s", path, namelist[i]->d_name);
            RemoveDirch(tmpPath);
        }
        if ((cnt = scandir(path, &namelist, NULL, alphasort)) == -1) {
            fprintf(stderr, "ERROR: scandir error for %s\n", path);
            return 1;
        }// 디렉터리 내에 더 이상 항목이 없는 경우 디렉터리 삭제
        if(cnt<3)remove(path);
    }
    else if(treeDir){
        remove(path);
    }
    return 0;
}
int RemoveCommand(command_parameter *parameter) {
    struct stat statbuf;
    int i;
    char *backuptime;
    char *originPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *backupPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *tmpPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char **backupPathList = NULL;
    int backupPathDepth = 0;
    int flag = 0;
    int filecnt = 0;
    int dircnt = 0;

    strcpy(originPath, parameter->filename);
    sprintf(backupPath, "%s%s", backupPATH, originPath + strlen(homePATH));

    if (lstat(originPath, &statbuf) < 0) {
        fprintf(stderr, "ERROR: lstat error for %s\n", originPath);
        return 1;
    }
    if (!S_ISREG(statbuf.st_mode) && !S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "ERROR: %s is not directory or regular file\n", originPath);
        return -1;
    }
    // 디렉터리인 경우 재귀적 삭제 여부 확인
    if (S_ISDIR(statbuf.st_mode) && !(parameter->commandopt & OPT_R) && !(parameter->commandopt & OPT_D)) {
        fprintf(stderr, "ERROR: %s is a directory\n - use \'-r, -d\' option or input in file path.\n", originPath);
        return -1;
    }
    if (parameter->commandopt & (OPT_R|OPT_D)) {
        flag = 1;
    }
    // 파일 또는 디렉터리 삭제 수행
    if (flag == 0) {
        RemoveFile(originPath, parameter->commandopt);
    } else {
        if (S_ISREG(statbuf.st_mode)) {
            if (parameter->commandopt & OPT_R || parameter->commandopt & OPT_D) {
                fprintf(stderr, "ERROR: %s is not directory\n", originPath);
                return -1;
            }
        }RemoveAll(originPath, flag, parameter->commandopt);
    }
    RemoveDirch(backupPATH);
    return 0;
}

int BackupFile(char *path, char *date, int commandopt) {
    int len;
    int fd1, fd2, log_fd;
    char *buf = (char *) malloc(sizeof(char *) * STRMAX);
    struct stat statbuf, tmpbuf;
    struct dirent **namelist;
    int cnt;
    char *filename = (char *) malloc(sizeof(char *) * PATHMAX);
    char *filepath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *tmpPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *tmpName = (char *) malloc(sizeof(char *) * PATHMAX);
    char *tmpdir = (char *) malloc(sizeof(char *) * PATHMAX);
    char *newPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *logpath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *dirrm = (char *) malloc(sizeof(char *) * PATHMAX);
    char *dirback = (char *) malloc(sizeof(char *) * PATHMAX);
    int idx;
    int i;

    char *filehash = (char *) malloc(sizeof(char *) * hash);
    char *tmphash = (char *) malloc(sizeof(char *) * hash);

    strcpy(filepath, path);
    for (idx = strlen(filepath) - 1; filepath[idx] != '/'; idx--);

    strcpy(filename, filepath + idx + 1);
    filepath[idx] = '\0';
    if(!commandopt || (commandopt == OPT_Y))strcpy(recurPATH,filepath);// 재귀 플래그가 설정되어 있는지 확인하고 재귀 경로 업데이트
    if (lstat(path, &statbuf) < 0) {
        fprintf(stderr, "ERROR: lstat error for %s\n", path);
        return 1;
    }

    ConvertHash(path, filehash);//절대경로로 변환
    timeList *logcurr = backuplist;
    while (!(commandopt & OPT_Y)) {//백업되어있는 거 확인
        logcurr = logcurr->next;
        if (!strcmp(logcurr->path, path)) {
            strcpy(tmpPath, logcurr->backuppath);
            tmpPath[strlen(logcurr->backuppath)] = 0;

            if (lstat(tmpPath, &tmpbuf) < 0) {
                fprintf(stderr, "ERROR: lstat error for %s\n", tmpPath);
                return 1;
            }
            if (S_ISREG(tmpbuf.st_mode)) {
                if (statbuf.st_size != tmpbuf.st_size) {
                    if (logcurr->next == NULL)break;
                    else continue;
                }
                ConvertHash(tmpPath, tmphash);
                if (!strcmp(filehash, tmphash)) {
                    printf("\"%s\" is already backuped to \"%s\"\n", path, tmpPath);
                    return 1;
                }
            }
        }
        if (logcurr->next == NULL) break;
    }

    strcpy(dirback, backupPATH);
    strcat(dirback, "/");
    strcat(dirback, date);//path 를 만들어서
    sprintf(newPath, "%s%s", dirback, path+strlen(recurPATH));

    if ((fd1 = open(path, O_RDONLY)) < 0) {
        fprintf(stderr, "ERROR: open error for %s\n", path);
        return 1;
    }

    if ((fd2 = open(newPath, O_CREAT | O_TRUNC | O_WRONLY, 777)) < 0) {
        fprintf(stderr, "ERROR: open error for %s\n", newPath);
        return 1;
    }

    while ((len = read(fd1, buf, statbuf.st_size)) > 0) {
        write(fd2, buf, len);
    }
    printf("\"%s\" backuped to \"%s\"\n", path, newPath);

    sprintf(logpath, "%s : \"%s\" backuped to \"%s\"\n", date, path, newPath);//backuplog 남기기
    len = strlen(path) + strlen(newPath) + strlen(date) + 21;
    logpath[len] = 0;

    if ((log_fd = open(ssubak, O_WRONLY | O_APPEND)) < 0) {//이어서 쓸 수 있게
        fprintf(stderr, "ERROR: open error for %s\n", ssubak);
        return 1;
    }
    write(log_fd, logpath, len);
    close(log_fd);
}

int BackupDir(char *path, char *date, int commandopt) {
    struct dirent **namelist;
    struct stat statbuf;
    char *tmppath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *tmpdir = (char *) malloc(sizeof(char *) * PATHMAX);
    int cnt;

    strcpy(tmpdir, backupPATH);
    strcat(tmpdir, "/");
    strcat(tmpdir, date);
    strcat(tmpdir, path+strlen(recurPATH));

    if (access(tmpdir, F_OK))
        mkdir(tmpdir, 0777);
    if ((cnt = scandir(path, &namelist, NULL, alphasort)) == -1) {// 디렉토리 내 파일 및 서브디렉토리 스캔
        fprintf(stderr, "ERROR: scandir error for %s\n", path);
        return 1;
    }

    for (int i = 0; i < cnt; i++) { // 각 파일 및 서브디렉토리를 백업
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;

        strcpy(tmppath, path);
        strcat(tmppath, "/");
        strcat(tmppath, namelist[i]->d_name);
        if (lstat(tmppath, &statbuf) < 0) {
            fprintf(stderr, "ERROR: lstat error for %s\n", tmppath);
            return 1;
        }

        if (S_ISDIR(statbuf.st_mode)&& (commandopt & OPT_R)) {
            dirNode *new = (dirNode *) malloc(sizeof(dirNode));// 디렉토리 목록에 서브디렉토리를 추가하기 위한 새 노드 생성
            strcpy(new->path, tmppath);
            mainDirList->tail->next = new;
            mainDirList->tail = mainDirList->tail->next;
        } else if (S_ISREG(statbuf.st_mode)) {
            if(!(commandopt & OPT_Y))BackupFile(tmppath, date,commandopt);
            else BackupFile(tmppath, date,commandopt);
        }
    }
}

int AddCommand(command_parameter *parameter) {
    struct stat statbuf;
    char *backuptime;
    char *tmpPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *originPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *newBackupPath = (char *) malloc(sizeof(char *) * PATHMAX);
    char *tmpdir = (char *)malloc(sizeof(char *) * PATHMAX);
    char *date = (char *)malloc(sizeof(char *) * PATHMAX);
    char **backupPathList = NULL;
    int backupPathDepth = 0;
    int i;

    strcpy(originPath, parameter->filename);

    if (lstat(originPath, &statbuf) < 0) {
        fprintf(stderr, "ERROR: lstat error for %s\n", originPath);
        return 1;
    }

    if (!S_ISREG(statbuf.st_mode) && !S_ISDIR(statbuf.st_mode)) {// 백업 대상이 디렉토리나 일반 파일이 아닌 경우 오류 처리
        fprintf(stderr, "ERROR: %s is not directory or regular file\n", originPath);
        return -1;
    }

    if (S_ISDIR(statbuf.st_mode) && !(parameter->commandopt & OPT_R) && !(parameter->commandopt & OPT_D)) {
        fprintf(stderr, "ERROR: %s is a directory\n - use \'-r or -d\' option or input in file path.\n", originPath);
        return -1;
    }

    sprintf(newBackupPath, "%s%s", backupPATH, originPath + strlen(homePATH));
    if ((backupPathList = GetSubstring(newBackupPath, &backupPathDepth, "/")) == NULL) {
        fprintf(stderr, "ERROR: %s can't be backuped\n", originPath);
        return -1;
    }
    sprintf(date, "%s", getDate());// 현재 날짜 및 시간을 가져와서 백업용 임시 디렉토리 경로 생성
    strcpy(tmpdir, backupPATH);
    strcat(tmpdir, "/");
    strcat(tmpdir, date);

    if(access(tmpdir, F_OK))
        mkdir(tmpdir, 0777);

    if (S_ISREG(statbuf.st_mode)) {
        if(parameter->commandopt&OPT_R || parameter->commandopt&OPT_D){
            fprintf(stderr, "ERROR: %s is not directory\n", originPath);
            return -1;
        }
        BackupFile(originPath, date, parameter->commandopt);
    } else if (S_ISDIR(statbuf.st_mode)) {// 백업 대상 디렉토리를 탐색하고 백업하는 함수 호출
        mainDirList = (dirList *) malloc(sizeof(dirList));
        dirNode *head = (dirNode *) malloc(sizeof(dirNode));
        mainDirList->head = head;
        dirNode *curr = head->next;
        dirNode *new = (dirNode *) malloc(sizeof(dirNode));
        strcpy(new->path, originPath);
        curr = new;
        mainDirList->tail = curr;

        while (curr != NULL) {
            if(!recursion){strcpy(recurPATH,curr->path);recursion=1;}
            BackupDir(curr->path, date, parameter->commandopt);

            curr = curr->next;
        }
    }
    RemoveDirch(backupPATH);
    return 0;
}

void CommandFun(char **arglist) {
    int (*commandFun)(command_parameter *parameter);
    command_parameter parameter = {
            arglist[0], arglist[1], arglist[2], atoi(arglist[3])
    };
    // 입력된 커맨드에 따라 적절한 함수를 할당
    if (!strcmp(parameter.command, commanddata[0])) {
        commandFun = AddCommand;
    } else if (!strcmp(parameter.command, commanddata[1])) {
        commandFun = RemoveCommand;
    } else if (!strcmp(parameter.command, commanddata[2])) {
        commandFun = RecoverCommand;
    }
    if (commandFun(&parameter) != 0) {
        exit(1);// 할당된 함수를 실행하고 오류가 발생한 경우 프로그램 종료
    }
}

void CommandExec(command_parameter parameter) {
    pid_t pid;

    parameter.argv[0] = "command";// argv 배열의 첫 번째 인자에 프로그램명 할당
    parameter.argv[1] = (char *) malloc(sizeof(char *) * 32);
    sprintf(parameter.argv[1], "%d", hash);

    parameter.argv[2] = parameter.command;
    parameter.argv[3] = parameter.filename;
    parameter.argv[4] = parameter.tmpname;
    parameter.argv[5] = (char *) malloc(sizeof(char *) * 32);
    sprintf(parameter.argv[5], "%d", parameter.commandopt);
    parameter.argv[6] = (char *) 0;

    if ((pid = fork()) < 0) {// fork로 자식 프로세스 생성
        fprintf(stderr, "ERROR: fork error\n");
        exit(1);
    } else if (pid == 0) {// execv를 사용하여 새로운 프로세스로 전환하여 커맨드 실행
        execv(exeNAME, parameter.argv);
        exit(0);
    } else {
        pid = wait(NULL);
    }
}

void SystemExec(int argc, char **arglist) {
    pid_t pid;
    char whichPath[PATHMAX];
    // 시스템 명령어의 경로 설정
    sprintf(whichPath, "/usr/bin/%s", arglist[0]);

    arglist[0] = whichPath;// 첫 번째 인자를 시스템 명령어의 경로로 설정

    if ((pid = fork()) < 0) {
        fprintf(stderr, "ERROR: fork error\n");
        exit(1);
    } else if (pid == 0) {
        execv(whichPath, arglist);
        exit(0);
    } else {
        wait(NULL);
    }
}

void ParameterInit(command_parameter *parameter) {//파라미터 생성
    parameter->command = (char *) malloc(sizeof(char) * PATH_MAX);
    parameter->filename = (char *) malloc(sizeof(char) * PATH_MAX);
    parameter->tmpname = (char *) malloc(sizeof(char) * PATH_MAX);
    parameter->commandopt = 0;
}

int ParameterProcessing(int argcnt, char **arglist, int command, command_parameter *parameter) {
    struct stat buf;
    optind = 0;
    opterr = 0;
    int lastind;
    int option;
    int optcnt = 0;

    switch (command) {
        case CMD_ADD: {
            if (argcnt < 2) {// 인자 부족할 때 오류 처리
                fprintf(stderr, "ERROR : missing operand <PATH>\n");
                return -1;
            }
            if (ConvertPath(arglist[1], parameter->filename) != 0) {
                fprintf(stderr, "ERROR: \'%s\' is not exist\n", parameter->filename);
                return -1;
            }
            if (strncmp(parameter->filename, HOMEPATH, strlen(homePATH))
                || !strcmp(parameter->filename, HOMEPATH)) {// 사용자 디렉토리 내부의 경로인지 확인
                fprintf(stderr, "ERROR: path must be in user directory\n - \'%s\' is not in user directory.\n",
                        parameter->filename);
                return -1;
            }

            if (lstat(parameter->filename, &buf) < 0) {
                fprintf(stderr, "2ERROR: lstat error for %s\n", parameter->filename);
                return -1;
            }

            if (!S_ISREG(buf.st_mode) && !S_ISDIR(buf.st_mode)) {// 정규 파일 또는 디렉토리인지 확인
                fprintf(stderr, "3ERROR: %s is not regular file\n", parameter->filename);
                return -1;
            }

            lastind = 2;
            // 옵션 처리
            while ((option = getopt(argcnt, arglist, "rdy")) != -1) {
                if (option != 'r'&&option != 'y'&&option != 'd') {
                    fprintf(stderr, "ERROR: unknown option %c\nUsage : backup <PATH> [OPTION]\n", optopt);
                    return -1;
                }

                if (optind == lastind) {
                    fprintf(stderr, "ERROR: wrong option input\n");
                    return -1;
                }
                // 각 옵션에 대한 처리
                if (option == 'r') {
                    if (parameter->commandopt & OPT_R) {
                        fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                        return -1;
                    }
                    parameter->commandopt |= OPT_R;
                }
                else if(option == 'y'){
                    if (parameter->commandopt & OPT_Y) {
                        fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                        return -1;
                    }
                    parameter->commandopt |= OPT_Y;
                }
                else if(option == 'd'){
                    if (parameter->commandopt & OPT_D) {
                        fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                        return -1;
                    }
                    parameter->commandopt |= OPT_D;
                }
                if((parameter->commandopt & OPT_R)&&(parameter->commandopt & OPT_D)) parameter->commandopt&=~OPT_D;
                optcnt++;
                lastind = optind;
            }
            if (argcnt - optcnt != 2) {// 옵션 처리 후 남은 인자 확인
                fprintf(stderr, "ERROR: argument error\n");
                return -1;
            }
            break;
        }
        case CMD_REM: {
            if (argcnt < 2) {// 인자 부족할 때 오류 처리
                fprintf(stderr, "Usage : %s <FILENAME> [OPTION]\n", arglist[0]);
                return -1;
            }

            parameter->filename = arglist[1];
            lastind = 1;
            // 옵션 처리
            while ((option = getopt(argcnt, arglist, "rda")) != -1) {
                if (option != 'r' && option != 'a' && option != 'd') {
                    fprintf(stderr, "ERROR: unknown option %c\nUsage : remove <PATH> [OPTION]\n", optopt);
                    return -1;
                }

                if (optind == lastind) {
                    fprintf(stderr, "ERROR: wrong option input\n");
                    return -1;
                }
                //각 옵션마다의 처리 == commandopt에 합산해주기
                if (option == 'r') {
                    if (parameter->commandopt & OPT_R) {
                        fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                        return -1;
                    }
                    parameter->commandopt |= OPT_R;
                }

                if (option == 'a') {
                    if (parameter->commandopt & OPT_A) {
                        fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                        return -1;
                    }
                    parameter->commandopt |= OPT_A;
                }
                if (option == 'd') {
                    if (parameter->commandopt & OPT_D) {
                        fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                        return -1;
                    }
                    parameter->commandopt |= OPT_D;
                }

                optcnt++;
                lastind = optind;
            }

            if (((parameter->commandopt & OPT_R) && argcnt - optcnt != 2)
                || ((parameter->commandopt & OPT_A) && argcnt - optcnt != 2)) {
                fprintf(stderr, "ERROR: argument error\n");
                return -1;
            }

            if (ConvertPath(parameter->filename, parameter->filename) != 0) {
                fprintf(stderr, "ERROR: %s is invalid filepath\n", parameter->filename);
                return -1;
            }

            if (strncmp(parameter->filename, HOMEPATH, strlen(homePATH))
                || !strncmp(parameter->filename, backupPATH, strlen(backupPATH))
                || !strcmp(parameter->filename, HOMEPATH)) {
                fprintf(stderr, "ERROR: %s can't be backuped\n", parameter->filename);
                return -1;
            }
            break;
        }
        case CMD_REC: {
            if (argcnt < 2) {// 인자 부족할 때 오류 처리
                fprintf(stderr, "Usage : %s <FILENAME> [OPTION]\n", arglist[0]);
                return -1;
            }
            if (ConvertPath(arglist[1], parameter->filename) != 0) {
                fprintf(stderr, "ERROR: %s is invalid filepath\n", parameter->filename);
                return -1;
            }
            // 홈 디렉토리, 백업 디렉토리 내부의 파일 경로인지 확인
            if (strncmp(parameter->filename, HOMEPATH, strlen(homePATH))
                || !strncmp(parameter->filename, backupPATH, strlen(backupPATH))
                || !strcmp(parameter->filename, HOMEPATH)) {
                fprintf(stderr, "ERROR: %s can't be backuped\n", parameter->filename);
                return -1;
            }

            lastind = 2;
            // 옵션 처리
            while ((option = getopt(argcnt, arglist, "drn:l")) != -1) {
                if (option != 'r' && option != 'n' && option != 'd' && option != 'l') {
                    if(optopt=='n')fprintf(stderr, "N option's NewPath Empty\n");
                    else fprintf(stderr, "ERROR: unknown option %c\nUsage : recover <PATH> [OPTION]\n", optopt);
                    return -1;
                }

                if (optind == lastind) {
                    fprintf(stderr, "ERROR: wrong option input\n");
                    return -1;
                }
                //각 옵션마다의 처리 == commandopt에 합산해주기
                if (option == 'r') {
                    if (parameter->commandopt & OPT_R) {
                        fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                        return -1;
                    }
                    parameter->commandopt |= OPT_R;
                }
                if (option == 'l') {
                    if (parameter->commandopt & OPT_L) {
                        fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                        return -1;
                    }
                    parameter->commandopt |= OPT_L;
                }
                if (option == 'd') {
                    if (parameter->commandopt & OPT_D) {
                        fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                        return -1;
                    }
                    parameter->commandopt |= OPT_D;
                }

                if (option == 'n') {
                    if (parameter->commandopt & OPT_N) {
                        fprintf(stderr, "ERROR: duplicate option -%c\n", option);
                        return -1;
                    }

                    if (optarg == NULL) {
                        fprintf(stderr, "ERROR: <NEWNAME> is null\n");
                        return -1;
                    }

                    if (ConvertPath(optarg, parameter->tmpname) != 0) {
                        fprintf(stderr, "ERROR: %s is invalid filepath\n", parameter->tmpname);
                        return -1;
                    }
                    // 새로운 파일명의 유효성 검사
                    if (strncmp(parameter->tmpname, HOMEPATH, strlen(homePATH))
                        || !strncmp(parameter->tmpname, backupPATH, strlen(backupPATH))
                        || !strcmp(parameter->tmpname, HOMEPATH)) {
                        fprintf(stderr, "ERROR: %s can't be backuped\n", parameter->tmpname);
                        return -1;
                    }

                    parameter->commandopt |= OPT_N;
                    optcnt++;
                }
                optcnt++;
                lastind = optind;
            }
            if (argcnt - optcnt != 2) {//남은 인자 확인
                fprintf(stderr, "argument error\n");
                return -1;
            }
            break;
        }
    }
}

int Prompt(int argcnt, char **arglist) {
    char input[100][STRMAX] = {0}, a = 0;
    int command = 0b1000000, in;
    command_parameter parameter = {(char *) 0, (char *) 0, (char *) 0, 0};
    // 명령어 식별
    if (!strcmp(arglist[0], commanddata[0])) {
        command = CMD_ADD;
    } else if (!strcmp(arglist[0], commanddata[1])) {
        command = CMD_REM;
    } else if (!strcmp(arglist[0], commanddata[2])) {
        command = CMD_REC;
    } else if (!strcmp(arglist[0], commanddata[3])) {
        command = CMD_SYS;
    } else if (!strcmp(arglist[0], commanddata[4])) {
        command = CMD_SYS;
    } else if (!strcmp(arglist[0], commanddata[5])) {
        command = CMD_SYS;
    } else if (!strcmp(arglist[0], commanddata[6])) {// 'help' 명령어 처리
        if (argcnt==2) help_opt(arglist[1]);
        else if(argcnt==1)help();
        else {
            fprintf(stderr, "Too many argument\n");
            return -1;
        }
        return 0;
    } else if (atoi(arglist[0]) - hash) {
        command = NOT_CMD;
    }

    if (command & (CMD_ADD | CMD_REM | CMD_REC)) {
        ParameterInit(&parameter);
        parameter.command = arglist[0];
        if (ParameterProcessing(argcnt, arglist, command, &parameter) == -1) {
            return 0;
        }
        CommandExec(parameter);

    } else if (command & CMD_SYS) {// 'sys' 명령어 처리
        char *treePATH=(char *)malloc(sizeof(char)*PATHMAX);
        char *treebuf = (char *)malloc(sizeof(char)*PATHMAX);
        char *treepath=(char *)malloc(sizeof(char)*PATHMAX);
        strcpy(treebuf,exePATH);
        int treech=0;
        if (argcnt == 2) {
            ConvertPath(arglist[1],treebuf);//tree 디렉으로 패스 변경하는 과정
            if (strncmp(treebuf, HOMEPATH, strlen(homePATH))
                || !strncmp(treebuf, backupPATH, strlen(backupPATH))
                || !strcmp(treebuf, HOMEPATH)) {
                fprintf(stderr, "ERROR: %s not user directory.\n", treebuf);
                return -1;
            }
            sprintf(treePATH, "%s/tree/%s", backupPATH, treebuf+strlen(HOMEPATH)+1);

            if(access(treePATH,F_OK)) {
                fprintf(stderr, "Backup Directory of %s is empty.\n", treebuf);
                return -1;}//접근 불가능하면 종료
            struct stat treestat;
            if (lstat(treePATH, &treestat) < 0){
                fprintf(stderr, "ERROR : lstat error\n");
                return -1;
            }
            if(S_ISDIR(treestat.st_mode)) ;//chdir(treePATH);//디렉테러이면 이동
            else if(S_ISREG(treestat.st_mode)){//파일이면 출력
                sprintf(treelist[treelistcnt++], "%s", treebuf);
                printf("%3d. %s\n  >> ", treelistcnt - 1, treebuf);
                treech=1;
            }
        }
        else if(argcnt!=1) return -1;//인자의 개수가 범위 밖이면 종료
        else {
            sprintf(treePATH, "%s/tree/%s", backupPATH, treebuf + strlen(HOMEPATH) + 1);
            if(access(treePATH,F_OK)) {
                fprintf(stderr, "Backup Directory is empty.\n");
                return -1;
            }//접근 불가능하면 종료
        }
        if(!treech) {//디렉터리이거나 인자가 없었을 때 실행
            chdir(treePATH);
            sprintf(treelist[treelistcnt++], "%s", treebuf);
            printf("%3d. %s\n", treelistcnt - 1, treebuf);
            list_tree(1, treePATH);//트리 출력
            chdir(homePATH);//다시 돌아오기
        }
        treeDir=1;
        RemoveDirch("/home/backup/tree");
        treeDir=0;
        for (int i = 0;; i++) {
            scanf("%s", input[i]);
            a = getchar();
            if (a == '\n') {
                a = i + 1;
                break;
            }
        }
        char **argv = malloc(sizeof(char *) * (a + 1));
        for (int i = 0; i < a; i++) argv[i] = input[i];//이중포인터로 바꿔주기 -> 했음
        argv[a] = 0;
        if (a<2){
            if(!strcmp(argv[0],"exit")) return 0;//종료
            else {
                fprintf(stderr, "Wrong Command\n");
                return -1;
            }
        }
        if (atoi(argv[1]) > treecnt || atoi(argv[1]) < 0) {
            fprintf(stderr, "Invalid number\n");
            return -1;
        }strcpy(argv[1], treelist[atoi(argv[1])]);
        if (!strcmp("vi", argv[0]) || !strcmp("vim", argv[0])) {//선택한 파일로 vi 실행
            SystemExec((int) a, argv);
        }
        else if (!strcmp("rm", argv[0])) {//선택한 파일로 remove 실행
            ParameterInit(&parameter);
            parameter.command="remove";
            ParameterProcessing(a, argv, CMD_REM, &parameter);
            RemoveCommand(&parameter);
        }
        else if (!strcmp("rc", argv[0])) {//선택한 파일로 recover 실행
            ParameterInit(&parameter);
            parameter.command="recover";
            ParameterProcessing(a, argv, CMD_REC, &parameter);
            RecoverCommand(&parameter);
        }
        else{
            fprintf(stderr, "Invalid command\n");
            return -1;
        }
        return 0;
    } else if (!command) {
        fprintf(stderr, "ERROR: invalid command -- '%s'\n./ssu_backup help : show commands for program\n", arglist[0]);
        return -1;
    }
}

void Init() {
    backuplist = (timeList *) malloc(sizeof(timeList));
    // 실행 파일의 경로 및 홈 디렉토리 경로 설정
    strcpy(HOMEPATH, "/home");
    getcwd(exePATH, PATHMAX);
    strcpy(homePATH, getenv("HOME"));
    snprintf(backupPATH,strlen(HOMEPATH)+8, "%s/backup", HOMEPATH);// 백업 디렉토리 경로 설정
    snprintf(ssubak,strlen(backupPATH)+12, "%s/ssubak.log",backupPATH);// 백업 로그 파일 경로 설정

    if (access(backupPATH, F_OK))// 백업 디렉토리가 존재하지 않을 경우 생성
        mkdir(backupPATH, 0777);
    chdir(backupPATH);

    int fd;
    if ((fd = open(ssubak, O_RDWR | O_CREAT, 0777)) < 0) {// 백업 로그 파일 열기
        fprintf(stderr, "open error for %s\n", ssubak);
        exit(1);
    }
    backuplist = Gettime_list();
    chdir(exePATH);// 실행 파일이 위치한 디렉토리로 이동
}

int main(int argc, char *argv[]) {
    Init();
    treemake();
    if (!strcmp(argv[0], "command")) {// "command"로 프로그램이 실행되었을 때
        hash = atoi(argv[1]);

        CommandFun(argv + 2);
    } else {
        if (argc < 2) {// 명령어 부족 오류
            fprintf(stderr, "ERROR: wrong input.\n%s help : show commands for program\n", argv[0]);
            return -1;
        }
    }
    strcpy(exeNAME, argv[0]);// 실행 파일 이름 복사
    hash = HASH_MD5;// 해시값 설정

    Prompt(argc - 1, argv + 1); // 프롬프트 함수 호출
    RemoveDirch("/home/backup/tree");
    exit(0);
}
