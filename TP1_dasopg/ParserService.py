import socket
import json
import csv
import signal  
import time
import traceback

class Moneda:
    def __init__(self, id, name, value1, value2):
        self.setMoneda(id, name, value1, value2)

    def setMoneda(self, id, name, value1, value2):
        self.id = id
        self.name = name
        self.value1 = value1
        self.value2 = value2

class Monedas:
    def __init__(self):
        self.__exchanges = []
        self.__filePath = ""

    def readFilePath(self):
        print(f"Reading: config.txt")
        try:
            with open('config.txt', "r", encoding="utf-8") as f:
                filePath = f.read()
                self.setFilePath(filePath)
        except FileNotFoundError as err:
            print('file does not exist')
            raise err
        except IOError as err:
            print('IO Error')
            raise err

    def getFilePath(self):
        return self.__filePath

    def setFilePath(self, filePath):
        self.__filePath = filePath

    def readCSV(self):
        filePath = self.getFilePath()
        print(f"Reading: {filePath}")
        try:
            with open(filePath, "r", encoding="utf-8") as f:
                data = csv.DictReader(f)
                for row in data:
                    e = Moneda(row["id"], row["name"], row["value1"], row["value2"], )
                    self.__exchanges.append(e)
        except FileNotFoundError as err:
            print('file does not exist')
            raise err
        except IOError as err:
            print('IO Error')
            raise err

    def getExchanges(self):
        return self.__exchanges

    def getExchangesJSON(self):
        jsonArray = []
        for exchange in self.getExchanges():
            jsonArray.append(exchange.__dict__)
        return json.dumps(jsonArray)

class Main:
    def __init__(self):
        self.__programmState = False
        self.__host = "127.0.0.1"
        self.__port = 10000

    def getState(self):
        return self.__programmState

    def setState(self, state):
        self.__programmState = state

    def handler(self,sig, frame):  # define the handler  
        print("Signal Number:", sig, " Frame: ", frame)  
        traceback.print_stack(frame)
        print("Ending programm from keyboard...")
        self.setState(False)
    
    def main(self):
        signal.signal(signal.SIGINT, self.handler)  # hacerlo en el thread ppal. El handler siempre se ejecuta en el thread ppal.

        # Init Exchanges
        e = Monedas()
        e.readFilePath()
        
        # Create UPD socket at client side
        try:
            print("Creating UPD client...")
            sClient = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
            self.setState(True)
        except:
            print("Failed creating UPD client...")

        while (self.getState()):  
            try:
                e.readCSV()
                msgToSend = e.getExchangesJSON()

                # Send to server
                sClient.sendto(str.encode(msgToSend),(self.__host, self.__port))
            except:
                print("Failed getting data to send...")

            time.sleep(30)

        # Close socket client
        sClient.close()
    
#Run programm
m = Main()
m.main()