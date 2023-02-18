#include <sys/time.h>

#include <time.h>
#include <stdio.h>

#define FOUR        a+=1; a+=b; a+=100; a+=3; //在这里构建你的指令序列吧
#define TWENTY      FOUR FOUR FOUR FOUR FOUR 
#define HUNDRED     TWENTY TWENTY TWENTY TWENTY TWENTY
#define THOUSAND    HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED
#define TENTHOUSAND THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND
#define MILLION     TENTHOUSAND TENTHOUSAND TENTHOUSAND TENTHOUSAND TENTHOUSAND TENTHOUSAND TENTHOUSAND TENTHOUSAND TENTHOUSAND TENTHOUSAND 
#define TENMILLION  MILLION MILLION MILLION MILLION MILLION MILLION MILLION MILLION MILLION MILLION


int main(){
	struct timespec begin,end;
register long long a = 0;
register long long b = 0; //加入几个寄存器，方便构建复杂的序列
register long long c = 0; //加入几个寄存器，方便构建复杂的序列
register int i = 1000000; //修改后需要更改计时的算法哦！因为实际执行的指令数量发生了变化
clock_gettime(CLOCK_REALTIME,&begin);
do{
   TENTHOUSAND //修改后需要更改计时的算法哦！因为实际执行的指令数量发生了变化
   i--;
} while (i);
clock_gettime(CLOCK_REALTIME,&end);
printf("%f", (10000000000)  / (end.tv_sec - begin.tv_sec + (end.tv_nsec - begin.tv_nsec)/1.0e9) / 1000000000);
return a == b;



}
