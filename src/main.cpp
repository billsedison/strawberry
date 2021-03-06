#include "parser.h"
#include "lexer.h"
#include "exception.h"
#include "ast_method.h"
#include "vm.h"
#include "assembler.h"
#include "disassembler.h"
#include <cstring>

#define GETOPT(S) get_option(argc,argv,S)
#define SETOUT(S) {if(!out)out=S;}

int get_option(int argc,char *argv[],const char *option)
{
    for(int i=2;i<argc;i++)if(argv[i][0]=='-'&&!strcmp(argv[i]+1,option))return i;
    return 0;
}

const char *tmpname()
{
    static char s[] = "XXXXXX";
    mkstemp(s);
    return s;
}
int main(int argc,char *argv[])
{
    if(argc==1)
    {
        printf("Usage: %s filename [options]\n",argv[0]);
        printf("\t -s compile script to assembly\n");
        printf("\t -x compile script to binary\n");
        printf("\t -c compile assembly to binary\n");
        printf("\t -o set output filename\n");
        return 0;
    }
    char *in=argv[1];
    const char *out=NULL;
    if(CodeData::check(in))
    {
        out=in;
    }
    else
    {
        int opt;
        if((opt=GETOPT("o")))out=argv[opt+1];
        bool s=GETOPT("s"),x=GETOPT("x"),c=GETOPT("c"),d=GETOPT("d");

        if(c)
        {
            SETOUT("out.bin")
            if(assemble(in,out))return 0;
            return 1;
        }
        if(d)
        {
            SETOUT("out.asm")
            if(disassemble(in,out))return 0;
            return 1;
        }
        Lexer lexer;
        if(!lexer.load(in))
        {
            fprintf(stderr,"cannot open %s\n",in);
            return 1;
        }
        if(s)SETOUT("out.asm")
        else if(x) SETOUT("out.bin")
        else SETOUT(tmpname())
        Parser parser(lexer);
        AST_PTR ast;
        const char *asm_out;
        try
        {
            ast=parser.parse();
            astMethodInit();
            ast->fill(nullptr);
            ast->check(nullptr);

            if(!x)asm_out=out;
            else asm_out=tmpname();
            FILE *fp=fopen(asm_out,"w");
            AST::fp=fp;
            ast->codeGen(nullptr);
            fclose(fp);
            if(s)return 0;
            int result=assemble(asm_out,out)?0:1;
            if(x||result)
            {
                remove(asm_out);
                return result;
            }
        }
        catch(Exception &e)
        {
            e.print(stderr);
            if(x)remove(asm_out);
            if(in!=out)remove(out);
            return 1;
        }

    }
    VirtualMachine vm;
    if(!vm.load(out))printf("cannot load %s\n",out);
    else if(in!=out)remove(out);;
    vm.run();
    return 0;
}

