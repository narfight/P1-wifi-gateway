
# P1 wifi gateway

Firmware de remontée d'information pour le module P1 Reader (voir https://github.com/romix123/P1-wifi-gateway)


## Installation

Télécharger la dernière version du firmware dans les builds et utilisé l'interface web du module pour envoyer le nouveau firmware.

Le module va créer un WiFi "P1_setup_XXXX" pour le configurer. Il aura l'IP 192.168.4.1. Vous pouvez donc aller sur http://192.168.4.1 pour configurer le module.

A la première connexion, le module va vous demander un login et un mot de passe. Si vous laisser les champs de mot de passe vide, le module ne sera pas protégé.

## Roadmap

- Ajouter des langues de traduction
- Update via l'interface web depuis Github
- Compatible avec d'autre pays que la Belgique


## Comment compiler votre version
Il faut :
* Visual Code
* PlatformIO

## Related

Voici le projet source pour le hardware et le software :
[romix123](https://github.com/romix123/P1-wifi-gateway)

## License
see <http://www.gnu.org/licenses/>.