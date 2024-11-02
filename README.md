# P1 Wi-Fi Gateway

Firmware pour le module P1 Reader, conçu pour capturer et transmettre des informations de comptage via Wi-Fi. Ce projet facilite l'intégration de données P1 dans des systèmes domotiques tels que Home Assistant et Domoticz.

N'oubliez pas de consulter le Wiki très complet : [Le Wiki](https://github.com/narfight/P1-wifi-gateway/wiki)

## Table des matières
- [Fonctionnalités](#fonctionnalités)
  - [Catpures d'écran](#catpures-d%C3%A9cran)
- [Achat du module](#Achat-du-module)
- [Prérequis](#prérequis)
- [Installation](#installation)
- [Configuration du Module](#configuration-du-module)
- [Utilisation](#utilisation)
- [Roadmap](#roadmap)
- [Contribution](#contribution)
- [Related](#related)
- [License](#license)

## Fonctionnalités

- **Transmission MQTT** : Envoie les données P1 aux serveurs domotiques via MQTT, compatible avec des plateformes comme Home Assistant et Domoticz.
- **Configuration via Wi-Fi** : Crée un point d'accès Wi-Fi (SSID : `P1_setup_XXXX`) pour configurer le module, accessible à l’adresse [http://192.168.4.1](http://192.168.4.1).
- **Support multilingue** : Interface multilingue avec possibilité d'ajouter des langues.
- **Mise à jour du firmware** : Mise à jour du firmware possible via l’interface web.
- **Compatibilité internationale** : Initialement conçu pour la Belgique, mais extensible pour d'autres pays sur demande
- **Intégration** : Compatible avec Home Assistant et Domoticz
### Catpures d'écran
<img src="https://github.com/user-attachments/assets/ae05256c-f895-4f6a-bbab-7d369eba7c81" width="400"/>
<img src="https://github.com/user-attachments/assets/3048b403-0873-40e3-9426-1f866c38b29c" width="50%"/>
<img src="https://github.com/user-attachments/assets/bbeff2c2-e0a4-48f6-986a-942417444dd0" width="400"/>
<img src="https://github.com/user-attachments/assets/0c39660f-1bcf-4faf-8f9e-087691f3a860" width="50%"/>

## Achat du module

Le module est développé par Ronald Leenes (ronaldleenes@icloud.com) et vous pouvez commander via son email le module de 22€ tout frais compris (en date de 2024).
Pour plus de détails sur la source du projet : http://www.esp8266thingies.nl/

## Prérequis

Pour compiler et déployer ce projet, vous aurez besoin des éléments suivants :
- [Visual Studio Code](https://code.visualstudio.com/) avec l’extension PlatformIO.
- Python 3.x (pour les scripts de compilation avancée).
- Une connexion au port P1 de votre compteur (souvent pour les compteurs intelligents).
- [Home Assistant](https://www.home-assistant.io/) ou [Domoticz](https://www.domoticz.com/) pour l’intégration domotique.

## Installation

1. **Téléchargement du Firmware** : Téléchargez la dernière version du firmware depuis la section des builds.
2. **Mise à jour du module** : Utilisez l’interface web du module pour télécharger et flasher le firmware.
3. **Connexion initiale** : Le module crée un réseau Wi-Fi `P1_setup_XXXX`. Connectez-vous à ce réseau.
4. **Accès à l'interface** : Accédez à l’interface de configuration via [http://192.168.4.1](http://192.168.4.1).

## Configuration du Module

1. **Authentification** : Lors de la première connexion, un login et un mot de passe sont demandés. Si les champs sont laissés vides, le module n’aura pas de protection par mot de passe.
2. **Paramètres réseau** : Configurez les informations du réseau Wi-Fi pour permettre au module de se connecter à Internet.
3. **Paramètres MQTT** :
   - **Serveur MQTT** : Indiquez l'adresse de votre serveur MQTT.
   - **Port** : Par défaut, le port est 1883.
   - **Identifiants** : Renseignez les identifiants si votre serveur MQTT est protégé.
4. **Intégration dans Home Assistant ou Domoticz** : Utilisez le fichier de configuration MQTT (`mqtt-P1Meter.yaml`) pour configurer facilement Home Assistant.

## Utilisation

Une fois configuré, le module commencera à envoyer des données de comptage via MQTT. Ces données peuvent inclure :
- **Consommation électrique en temps réel**.
- **Historique de consommation** : Le module peut enregistrer des données de consommation pour analyse.
- **Alertes personnalisées** : Configurez des alertes dans Home Assistant ou Domoticz pour surveiller des seuils de consommation.

### Surveillance et Diagnostics

Le module propose des outils de diagnostic accessibles via l’interface web, où vous pouvez consulter :
- **Journal des événements** : Suivi des connexions et erreurs.
- **Informations réseau** : Vérifiez la force du signal Wi-Fi et l’état de la connexion.
- **Logs MQTT** : Consulter les messages envoyés et reçus via MQTT.

## Roadmap

- **Support multilingue étendu** : Un firmware par langue
- **Compatibilité internationale** : Adaptation pour une compatibilité plus large en dehors de la Belgique.

## Contribution

Les contributions sont les bienvenues ! Veuillez suivre les étapes ci-dessous :
1. Forkez le projet et clonez-le en local.
2. Créez une branche pour votre fonctionnalité (`git checkout -b nouvelle-fonctionnalité`).
3. Effectuez vos modifications et testez-les.
4. Envoyez une Pull Request pour examen.

## Related

Pour plus d'informations sur le projet matériel et logiciel original : [romix123 sur GitHub](https://github.com/romix123/P1-wifi-gateway)

## License

Ce projet est distribué sous la licence GNU General Public License. Voir [GNU General Public License](http://www.gnu.org/licenses/) pour plus de détails.
