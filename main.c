#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <ctype.h>
//用C語言打造組譯器
//基礎的程式設計 : 流水線結構
//讀檔案，先開啟檔案
/*
 * 掃描源文件兩次
 * 第一次掃描要求出所有標示符所代表的指令位址
 * 第二次掃描將源文件逐行翻譯成機器語言
 * */
//組譯完成，關閉檔案

#define HASHTABLE_MAX_SIZE 10000
struct HashNode* OP_Table[HASHTABLE_MAX_SIZE];
struct HashNode* Label_Table[HASHTABLE_MAX_SIZE];
struct HashNode* Pseudo_Instructions_Table[HASHTABLE_MAX_SIZE];

//手撕Hash Table，可以跳過直接到189行
struct HashNode {
    //我就用單純的鏈表吧
    //再手撕紅黑樹就太複雜了
    char* key;
    size_t value;
    struct HashNode* i_next;
};
unsigned int hashfunction(const char* key) {
    const signed char *p = (const signed char *) key;
    unsigned int h = (unsigned char)*p;
    if (h) {
        for (p += 1; *p != '\0'; ++p){
            h = (h << 5) - h + *p;
        }
    }
    return h;
}

struct HashNode* getKVP(struct HashNode* table[], char* k){
    //取得哈希值
    unsigned int pos = hashfunction(k) % HASHTABLE_MAX_SIZE;
    //如果不為空
    if(table[pos])
    {
        //先存到臨時變數，因為有可能哈希衝突
        struct HashNode* head = table[pos];
        //如果不為空
        while(head)
        {
            //如果形參的鍵剛好等於目前節點的鍵，就代表找到了，回傳
            if(strcmp(k, head->key) == 0){
                return head;
            }
            //否則就去確認下一個是不是
            head = head->i_next;
        }
    }
    //找不到就回傳null
    return NULL;
}

void hashtable_insert(struct HashNode* table[], const char* key, size_t value)
{
    //取得哈希值
    unsigned int pos = hashfunction(key) % HASHTABLE_MAX_SIZE;
    //head指向哈希值代表的位置
    struct HashNode* head =  table[pos];
    //不為空就代表哈希衝突，用鏈表處理
    while(head)
    {
        //先確認要插入的鍵值對不是重複的
        if(strcmp(head->key, key) == 0)
        {
            printf("%s already exists!\n", key);
            return ;
        }
        //指向下一個
        head = head->i_next;
    }
    //申請hashnode空間
    struct HashNode* Node = (struct HashNode*)malloc(sizeof(struct HashNode));
    //先清0
    memset(Node, 0, sizeof(struct HashNode));
    //申請字串空間
    Node->key = (char*)malloc(sizeof(char) * (strlen(key) + 1));
    //把形參的key字串複製過去
    strcpy(Node->key, key);
    //把形參的value複製過去
    Node->value = value;
    //讓這個node的next指向原本陣列中哈希值對應位置的第一個node
    Node->i_next = table[pos];
    //然後把現在插入的node變成第一個node
    table[pos] = Node;
    //總之就是現在插入的這個node是插在鏈表的頭
}

void table_release(struct HashNode* table[])
{
    //對陣列每一個位置所存的每一個節點都做空間釋放
    int i;
    for(i = 0; i < HASHTABLE_MAX_SIZE; ++i)
    {
        if(table[i])
        {
            struct HashNode* head = table[i];
            while(head)
            {
                struct HashNode* tmp = head;
                //一定要在釋放之前將head指向下一個
                head = head->i_next;
                free(tmp->key);
                free(tmp);
            }
        }
    }
}

void tables_init()
{
    //初始化全設為0
    memset(OP_Table, 0, sizeof(struct HashNode*) * HASHTABLE_MAX_SIZE);
    memset(Label_Table, 0, sizeof(struct HashNode*) * HASHTABLE_MAX_SIZE);
    //將指令與對應的OP code加入OP table
    /**
     * 我們班上的諾貝爾物理獎同學提供了指令對應OP code的json檔\n
     * 但我堂堂C語言原生不支援存取區區json檔\n
     * 沒辦法，只能手撕了，哭
     * */
    FILE *optable;
    optable = fopen(".\\optable.json","r");
    if(optable == NULL){
        char path[200];
        getcwd(path,200);
        printf("Failed to open optable.json at %s",path);
        exit(0);
    }
    //每次讀取一行
    while(!feof(optable)){
        char line[15];
        fgets(line,15,optable);
        char* pos = strchr(line,':');
        //現在這行如果搜尋不到冒號就直接忽略
        if(pos){
            //在json檔中的格式是"指令":"OPCODE"
            //在冒號的位置往後移兩位就是OPCODE的第一碼
            //但是要指定是16進制
            *pos = '0';
            *(pos+1) = 'x';
            //所以現在pos是opcode第一位
            //第一個引號的位置往後一位就是指令的第一個字母
            char* ins_pos = strchr(line,'\"');
            ins_pos++;
            //原本冒號的位置往前一位就是指令的最後一個位置往後一位，插結束符
            *(pos-1) = '\0';
            //opcode固定兩位，所以原本冒號的位置往後四位就是該插結束符的位置
            *(pos+4) = '\0';
            //現在兩個字串都有了，下一步是加入OP table
            char* endptr;
            //以16進制解析OPCODE
            int opcode = strtol(pos,&endptr,16);
            hashtable_insert(OP_Table,ins_pos,opcode);
        }
    }
    //讀入假指令們
    //其實我也不知道為什麼課本叫它assembler directives
    //但我學組合語言的時候用的名詞是偽指令
    //那就以我學過的為準吧，我寫著也比較習慣，叫pseudo_instruction
    fclose(optable);
    FILE* PIs = fopen(".\\pseudo_instructions","r");
    if(PIs == NULL){
        char path[200];
        getcwd(path,200);
        printf("Failed to open pseudo_instructions at %s",path);
        exit(0);
    }
    while(!feof(PIs)){
        char single_PI[20];
        fgets(single_PI,20,PIs);
        char* pi = strtok(single_PI,":");
        char* mulchr = strtok(NULL,":");
        char* endptr;
        int mul = strtol(mulchr,&endptr,10);
        //存假指令的value比較不一樣，指的是要乘以的倍數
        hashtable_insert(Pseudo_Instructions_Table,pi,mul);
    }
    fclose(PIs);
}

//目前的loc
size_t loc = 4096;
//寫個struct存第一遍掃描後每一行的資訊
struct Single_Line{
    struct HashNode* Label;
    char* instruction;
    char* operand;
    size_t m_loc;
    struct Single_Line* next;
};

struct Single_Line* insert_single_line(struct HashNode* label, char* instruction, char* operand, size_t l, struct Single_Line* prev){
    struct Single_Line* newLine = (struct Single_Line*) malloc(sizeof(struct Single_Line));
    memset(newLine,0,sizeof(struct Single_Line));
    newLine->Label = label;
    newLine->instruction = (char*)malloc(sizeof(char)*strlen(instruction)+1);
    strcpy(newLine->instruction,instruction);
    newLine->operand = (char*) malloc(sizeof(char)*strlen(operand)+1);
    strcpy(newLine->operand,operand);
    if(!strcmp(instruction,"END")){
        newLine->m_loc = -1;
    }else{
        newLine->m_loc = l;
    }
    newLine->next = NULL;
    prev->next = newLine;
    return newLine;
}

void printLine(struct Single_Line* current){
    char* label;
    if(current->Label != NULL){
        label = current->Label->key;
    }else{
        label = "\0";
    }
    char* endptr;
    size_t OPCODE;
    char opcodestring[7];
    if(getKVP(OP_Table,current->instruction) !=NULL){
        //如果這個指令在OP table中
        size_t op = getKVP(OP_Table,current->instruction)->value;
        if(strcmp(current->operand,"\0") == 0){
            sprintf(opcodestring,"%02zX%s",op,"0000");
            printf("%-4zX\t%-8s%-10s%-15s%s",current->m_loc,label,current->instruction,"",opcodestring);
            return ;
        }
        //如果instruction format中的x是1
        char* t_twoOperandPtr;
        if((t_twoOperandPtr = strchr(current->operand,',')) != NULL){
            *t_twoOperandPtr = '\0';
            sprintf(opcodestring,"%02zX%4zX",op, getKVP(Label_Table,current->operand)->value);
            opcodestring[2]+=8;
            *t_twoOperandPtr = ',';
            printf("%-4zX\t%-8s%-10s%-15s%s",current->m_loc,label,current->instruction,current->operand,opcodestring);
            return ;
        }
        //如果operand只有一個，跑以下流程
        size_t address = getKVP(Label_Table,current->operand)->value;
        sprintf(opcodestring,"%02zX%4zX",op,address);
        OPCODE = strtol(opcodestring,&endptr,16);
        if(current->m_loc == -1){
            printf("\t%-8s%-10s%-15s%06zX",label,current->instruction,current->operand,OPCODE);
        }
        else{
            printf("%-4zX\t%-8s%-10s%-15s%06zX",current->m_loc,label,current->instruction,current->operand,OPCODE);
        }
    }
    else if(getKVP(Pseudo_Instructions_Table,current->instruction) != NULL){
        //如果是假指令
        if(strcmp(current->instruction,"BYTE") == 0){
            //是Byte，可能會是C或X
            char* ptr = current->operand;
            if(*ptr =='X'){
                ptr+=2;
                char* str = ptr;
                while(isalnum(*ptr)){
                    ptr++;
                }
                *ptr = '\0';
                OPCODE = strtol(str,&endptr,16);
                printf("\t%-8s%-10s%-15s%06zX",label,current->instruction,current->operand,OPCODE);
                return ;
            }else if(*ptr == 'C'){
                ptr+=2;
                printf("\t%-8s%-10s%-15s%X%X%X",label,current->instruction,current->operand,*ptr,*(ptr+1),*(ptr+2));
                return ;
            }

        }else if(strcmp(current->instruction,"WORD") == 0){
            //是WORD，操作數十進制，OPCODE欄位16進制
            sprintf(opcodestring,"%s",current->operand);
            OPCODE = strtol(opcodestring,&endptr,10);
            printf("\t%-8s%-10s%-15s%06zX",label,current->instruction,current->operand,OPCODE);
            return ;
        }else{
            printf("\t%-8s%-10s%-15s",label,current->instruction,current->operand);
            return ;
        }
    }
}

size_t get_len(char* instruction, char* operand){
    if(getKVP(Pseudo_Instructions_Table,instruction)){
        //是假指令
        struct HashNode* target = getKVP(Pseudo_Instructions_Table,instruction);
        if(!strcmp(target->key,"START")){
            char *endptr;
            loc = strtol(operand,&endptr,16);
            return 0;
        }else if(!strcmp(target->key,"END")){
            return 0;
        }else if(!strcmp(target->key,"RESB")){
            char *endptr;
            size_t mul = strtol(operand,&endptr,10);
            return ((target->value)*mul);
        }else if(!strcmp(target->key,"RESW")){
            char *endptr;
            size_t mul = strtol(operand,&endptr,10);
            return ((target->value)*mul);
        }
        else if(!strcmp(target->key,"BYTE")){
            char* tmp = strrchr(operand,'\'');
                operand += 2;
                *tmp = '\0';
                //一個英文字是一個Byte
                //一個16進制數字是半個Byte
                if(*(operand-2) == 'X'){
                    return strlen(operand)/2;
                }else if(*(operand-2) == 'C'){
                    return strlen(operand);
                }
        }else{
            return target->value;
        }

    }else if(*instruction == '+'){
        //format4
        return 4;
    }else{
        //format3
        return 3;
    }
}

int main(int argc, char* argv[]) {
/*
    //使用方式: >>>組譯器 input output
    if(argc != 3){
        printf("Usage: %s input output\n",argv[0]);
        return 0;
    }
*/
    FILE *in,;
    //打開文件，讀取模式
    in = fopen(".\\figure","r");
    //打開文件，寫入模式，若文件不存在就創建
    //沒有檔案就報錯
    if(in == NULL){
        printf("Failed to open Input file.\n");
        return 0;
    }
    //初始化
    tables_init();
    /**
     * 第一遍掃描，目的是要知道標示符到底應該在甚麼位址
     * 為了達成這個目的，就需要知道每個指令有多長
     * 順便把一些該存的東西存一存
     * 所以先定義一些要存的臨時變數
     * */
    //虛擬頭節點(dummy head)
    struct Single_Line* shead = (struct Single_Line*)malloc(sizeof(struct Single_Line));
    //因為要逐漸插入新的行，所以需要一個指向鏈表最後一個元素的指針，初始是dummy head
    struct Single_Line* current_sline = shead;
    while(!feof(in)){
        char line[100];
        //讀取一行，存到line
        fgets(line, 100, in);
        //以空格分隔
        char* pos;
        pos = strtok(line," \t");
        //先處理特殊情況，如果是註釋的話要跳過
        if(*pos == '.'){
            continue;
        }
        //不一定有Label，先等於NULL
        struct HashNode* label = NULL;
        //現在pos是一行中的第一個元素，可能是標示符也可能是指令
        //如果不在op table及假指令table中代表是標示符，push進標示符的table中
        //否則就是指令，取得長度
        if(getKVP(OP_Table,pos) == NULL && getKVP(Pseudo_Instructions_Table,pos) == NULL) {
            hashtable_insert(Label_Table, pos, loc);
            label = getKVP(Label_Table,pos);
            //取得指令
            pos = strtok(NULL, " \t");
        }
        //某些假指令的長度取決於後面的operand，所以還得再讀
        char* pos2 = strtok(NULL," \t");
        char* nextlineptr = strrchr(pos2,'\n');
        if(nextlineptr != NULL){
            *nextlineptr = '\0';
        }
        size_t len = get_len(pos,pos2);
        current_sline = insert_single_line(label, pos, pos2, loc, current_sline);
        loc += len;
    }
    fclose(in);
    /**
     * 第二遍掃描
     * 要開始編OPCODE，也就是要組譯每一條語句
     * 針對一般的指令，都是兩位指令的OPCODE加上四位操作數的LOC
     * 而針對假指令，如果是START、END以及RES類的都不編
     * 如果是BYTE則判斷是文字(C)或數字(X)
     * 是文字則轉成ASCII碼
     * 是數字則直接存(HEX)
     * 如果是WORD就直接把後面的數字轉成HEX並儲存
     * 當然，因為時間太趕，我沒有把pass2的整體架構寫出來
     * 而是擠在了printLine裡，這是不太好的示範
     * 但我也沒什麼時間優化了，哭
     * */

    current_sline = shead->next;
    printf("%-4s\t%22s\t\t%11s\n","Loc","Source Statement","Object code");
    while(current_sline != NULL){
        //最後印到console
        printLine(current_sline);
        printf("\n");
        current_sline = current_sline->next;
    }
    //釋放資源
    table_release(OP_Table);
    table_release(Label_Table);
}