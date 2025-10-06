#include<stdio.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<ctype.h>


int main(int argc, char* argv[])
{

  if(argc == 1)
  {
    printf("You forgot to add arguments!\n");
    return 1;
  }
  char* input = argv[1];
  

  if(isalpha(input[0]))
  {
    struct hostent *host = gethostbyname(input);
    if(host == NULL)
    {
      printf("Not found information\n");
      return 1;
    }
    printf("Official IP: %s\n",inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
    printf("Alias IP: \n");
    for(int i = 1; host->h_addr_list[i] != NULL; i++)
    {
      char* ip = inet_ntoa(*(struct in_addr*)host->h_addr_list[i]);
      printf("%s\n", ip);
    }
  }
  else{
  struct in_addr addr;
    if (inet_pton(AF_INET, input, &addr) != 1) {
        printf("Invalid address\n");
        return 1;
    }

    struct hostent *host = gethostbyaddr(&addr, sizeof(addr), AF_INET);
    if (host == NULL) {
        printf("Not found information\n");
        return 1;
    }

    // Official name
    if (host->h_name)
        printf("Official name: %s\n", host->h_name);
    else
        printf("Official name: (none)\n");

    // Aliases (h_aliases may be NULL)
    if (host->h_aliases) {
        printf("Alias name(s):\n");
        for (int i = 0; host->h_aliases[i] != NULL; i++) {
            printf("  %s\n", host->h_aliases[i]);
        }
    } else {
        printf("Alias name: (none)\n");
    }
    
}

  return 0;
}