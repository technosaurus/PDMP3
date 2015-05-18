void pdmp3(char * const *mp3s);
int main(int ac, char **av){
  if (ac < 2) return 1;
  pdmp3(++av); //don't pass the command name
  return 0;
}
