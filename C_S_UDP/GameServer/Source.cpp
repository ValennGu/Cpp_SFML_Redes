#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <cstring>

#define MAX_PLAYERS 4
sf::UdpSocket socket;

struct Movment
{
	float movX = 0, movY = 0;
	int IDMove;
};
struct Player
{
	sf::IpAddress IP;
	unsigned short port;
	bool connected = true;
	int ID;
	float posX, posY;
	std::string name;
	sf::Clock timePing;
	std::map<int, Movment> movment;
	std::vector<Movment>interpolation;
};

int ID;
std::map<int, Player> Players;
float maxY = 480;
float minY = 1.f;
float maxX = 480.f;
float minX = 1.f;
//StateModes --> chat_mode - countdown_mode - bet_money_mode - bet_number_mode - simulate_game_mode - bet_processor_mode

void Recorrer() {
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		for (std::map<int, Player>::iterator it2 = Players.begin(); it2 != Players.end(); ++it2) {
			sf::Packet pack;
			pack << it2->first << it2->second.posX << it2->second.posY;
			if (socket.send(pack, it->second.IP, it->second.port) != sf::Socket::Done) {
				std::cout << "error";
			}
		}
	}

}

void Connection() {
	srand(time(NULL));
	sf::Packet packetLog;
	sf::Packet newPlayerPack;

	socket.bind(50000);
	Player player;
	for (int i = 0; i < MAX_PLAYERS; i++) {
		sf::IpAddress IP;
		unsigned short port;
		if (socket.receive(packetLog, IP, port) != sf::Socket::Done) {
			std::cout << "Error al recivir" << std::endl;
		}
		packetLog >> player.name;
		player.IP = IP;
		player.port = port;
		player.posX = rand() % 480;
		player.posY = rand() % 480;
		player.ID = i;

		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			std::string cmd = "CMD_NEW_PLAYER";
			newPlayerPack << cmd << player.name  << player.ID << player.posX << player.posY;
			if (socket.send(newPlayerPack, it->second.IP, it->second.port) != sf::Socket::Done) {
				std::cout << "Error al enviar nueva conexion" << std::endl;
			}
			newPlayerPack.clear();
		}

		Players.insert(std::pair<int, Player>(player.ID, player));
		packetLog.clear();
		//ID = i;
		packetLog << (int)Players.size();

		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
			std::string cmd = "CMD_WELCOME";
			packetLog << cmd << it->second.name << it->first << it->second.posX << it->second.posY;
			std::cout << "Se ha conectado : " << it->second.name << cmd << it->first << it->second.posX << it->second.posY << std::endl;
		}
		if (socket.send(packetLog, IP, port) != sf::Socket::Done) {
			std::cout << "error";
		}
		packetLog.clear();
	}
	
}
void sendAllPlayers( int id) {
	sf::Packet packDes;
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		std::string cmd = "CMD_DESC";
		packDes << cmd << id;
		if (socket.send(packDes, it->second.IP, it->second.port) != sf::Socket::Done) {
			std::cout << "Error al enviar nueva conexion" << std::endl;
		}
		packDes.clear();
	}
}

void Game() {
	for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
		it->second.timePing.restart();
	}
	socket.setBlocking(false);
	sf::IpAddress IP;
	unsigned short port;
	bool send = false;
	sf::Packet packR, packPingS;
	sf::Clock clockP;
	clockP.restart();
	std::string ping;
	sf::Packet packM;
	sf::Clock clockMov, clockAcum;
	clockMov.restart();
	clockAcum.restart();
	int id;
	std::string cmd;
	while (true) {
		if (clockP.getElapsedTime().asMilliseconds() > 1000) {
			for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); it++) {
				ping = "CMD_PING";
				packPingS << ping;
				if (socket.send(packPingS, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al enviar el ping" << std::endl;
				}
				else {
					clockP.restart();
					packPingS.clear();
				}
			}
		}
		for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); it++) {
			std::cout << Players.find(it->first)->first;
			if (it->second.timePing.getElapsedTime().asSeconds() >= 5) {
				//std::cout << "Eliminando" << it->first;
				sendAllPlayers(it->first);
				//std::cout << "Desconexion con la ID: " << it->first << std::endl;
				//std::cout << it->first;
				it->second.connected = false;
				//Players.erase(it);

			}
		}

		for (int i = 1; i <= Players.size(); i++) {
			if (Players.find(i) != Players.end() && !Players[i].connected) {
				//std::cout << "Client " << std::to_string(Players[i].id) << " disconnected." << std::endl;
				Players.erase(Players[i].ID);
			}
		}
		if (socket.receive(packR, IP, port) != sf::Socket::Done) {
		}
		else {
			packR >> cmd;
			if (cmd == "CMD_ACK") {
				packR >> id;
				Players.find(id)->second.timePing.restart();
			}
			if (cmd == "CMD_MOV") {
				int idMove, id;
				float deltaX, deltaY;
				packR >> idMove >> id >>  deltaX >> deltaY;
				std::string a = "CMD_OK_MOVE";
				packM << a;
				for (std::map<int, Player>::iterator it2 = Players.begin(); it2 != Players.end(); it2++) {
					if (it2->first == id) {
						Movment movAux;
						movAux.IDMove = idMove;
					//	std::cout << movAux.IDMove << std::endl;
						movAux.movX += deltaX;
						movAux.movY += deltaY;
						it2->second.movment.insert((std::pair<int, Movment>(idMove, movAux)));
						if (clockAcum.getElapsedTime().asMilliseconds() > 100) {

							if ((it2->second.posX += deltaX) > maxX) {
								it2->second.posX = maxX;
								packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
							}
							else if ((it2->second.posX += deltaX) < minX) {
								it2->second.posX = minX;
								packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
							}
							else if ((it2->second.posY += deltaY) > maxY) {
								it2->second.posY = maxY;
								packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
							}
							else if ((it2->second.posY += deltaY) < minY) {
								it2->second.posY = minY;
								packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
							}
							else {
								//acmular aqui		
								it2->second.posX += it2->second.movment[it2->second.movment.size() - 1].movX;
								it2->second.posY += it2->second.movment[it2->second.movment.size() - 1].movY;
								packM << it2->first << it2->second.movment[it2->second.movment.size() - 1].IDMove << it2->second.posX << it2->second.posY;
							//	std::cout << " X " << it2->second.movment[it2->second.movment.size() - 1].movX << " Y " << it2->second.movment[it2->second.movment.size() - 1].movY << std::endl;
								//std::cout << " IDM " << it2->second.movment[it2->second.movment.size() - 1].IDMove
									//<< " X " << it2->second.movment[it2->second.movment.size() - 1].movX << " Y " << it2->second.movment[it2->second.movment.size() - 1].movY;//<< " posX " << it2->second.posX << " posY " << it2->second.posY;		
							}
							clockAcum.restart();
						}
						for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); it++) {
							if (socket.send(packM, it->second.IP, it->second.port) != sf::Socket::Done) {
								std::cout << "Error al enviar mov" << std::endl;
							}
							//std::cout << it->second.name << "  " << it2->second.movment[it2->second.movment.size() - 1].IDMove <<  std::endl;
						}
						it2->second.movment.erase(it2->second.movment[it2->second.movment.size() - 1].IDMove);
						packM.clear();
					}
				}

			}
		}
		
		//it2->second.movment.erase(it2->second.movment.begin(), it2->second.movment.end());

		//}
		/*if (clockMov.getElapsedTime().asMilliseconds() > 200) {
			//std::cout << "Hola";
			for (std::map<int, Player>::iterator it = Players.begin(); it != Players.end(); ++it) {
				if (socket.send(packM, it->second.IP, it->second.port) != sf::Socket::Done) {
					std::cout << "Error al enviar mov" << std::endl;
				}
				//std::cout << it->second.movment.IDMove << it->second.posX << it->second.posY<< std::endl;
			}
			clockMov.restart();
			packM.clear();
		}*/
		
		packR.clear();
	}
}

int main()
{
	Connection();
	do {
		Game();
	} while (Players.size() >= 0);
	return 0;
}