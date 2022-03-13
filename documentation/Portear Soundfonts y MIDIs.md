# Archivos Importantes
- `direct_sound_data.inc`: Contiene todos los directorios de todos los archivos
- `voicegroups`: Es la asignación de un set de instrumentos a una canción
- `charmap.txt`: Solo hay que seguir la patrón de manera aditiva
- `ld_script.txt`: Reportamos la existencia de un nuevo midi
- ``songs.mk``: Definimos el volumen y el **voicegroup** que utilizarán los midis que agreguemos.
- `songs.h`: Aquí le damos un ID a una canción
- `song_table_inc`: Agregamos la canción a la tabla. Esta permite invocar canciones mediante macros

# Ruta para Añadir Soundfonts:

1) Copiamos todos los archivos de la carpeta ``direct_sound_samples`` al proyecto base
2) Copiamos textual el archivo de `direct_sound_data.inc` al proyecto original
3) 