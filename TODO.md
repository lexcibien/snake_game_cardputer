# TODO

## Lógica do jogo

- [ ] Pegar o código de desenhar a bateria do Launcher
- [X] Corrigir defeito como o cartão sd (funcionou quando o firmware é do m5launcher)
- [X] Corrigir rerender desnecessários (scoreboard)
  - [X] Diminuir o tamanho da janela
  - Cobra passa por cima porque é renderizada mais vezes que o texto
  - Pode ser o fundo que renderiza a cada vez que a cobra se move
  - Fruta se aparecer perto do scoreboard fica pela metade
- [ ] Separar em mais arquivos (seguir exemplo do outro snake)
- [ ] Implementar uma função no launcher para pegar o tema e o app buscar esse tema
- [ ] Adicionar um github actions
  - [ ] Lançar nova versão
  - [ ] Enviar versão para o M5Burner
  - [ ] Criar uma release
- [ ] Fazer um som quando atingir o highScore (mesmo do fim de jogo)
- [ ] Mudar o som de fim de jogo (descendo)
