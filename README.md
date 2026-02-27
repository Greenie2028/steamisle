# SteamIsle

Used to compile all games from a given users steam library into a single text document, providing quick and easy access to finding what Archipelago Games you have avaliable to you.

## Set Up

To run SteamIsle you need 
- steamisle executable
- Steam Web API Key
- - This can be acquired for free at https://steamcommunity.com/dev/apikey using the domain name "localhost".
- steamisle.cfg
- - Install steamisle.cfg.template, rename the file and replace YOUR_API_KEY with the key acquired above.


Usage:
```
./steamisle {steamId}
```
Returns a file named {steamUsername}.txt

- Note: User must have Steam profile public or else program will fail to run.