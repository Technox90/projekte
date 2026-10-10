"""BBQ-Chaos-Deutschland: 24/7 Discord radio stream relay."""
import asyncio
import logging
import os
import json
from pathlib import Path
import discord
from discord.ext import commands
from nowplaying import run_nowplaying
from commands import setup_commands

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger("bbq-radio")

TOKEN = os.environ["DISCORD_BOT_TOKEN"]
CHANNEL_ID = int(os.environ["DISCORD_CHANNEL_ID"])
STREAM_URL = os.environ["RADIO_STREAM_URL"]
VOLUME = float(os.getenv("RADIO_VOLUME", "0.1"))
if not 0.0 <= VOLUME <= 2.0:
    raise ValueError("RADIO_VOLUME muss zwischen 0.0 und 2.0 liegen")


class RadioBot(commands.Bot):
    def __init__(self):
        super().__init__(command_prefix='!', intents=discord.Intents.default())
        self.watchdog_task = None
        self.nowplaying_task = None
        self.voice_lock = asyncio.Lock()
        self.paused_by_user = False
        self.volume_file = Path(__file__).with_name('volume-state.json')
        self.volume = VOLUME
        if self.volume_file.exists():
            try:
                self.volume = float(json.loads(self.volume_file.read_text())['volume'])
            except (ValueError, KeyError, TypeError, OSError):
                log.warning('Lautstaerke-Datei ungueltig, verwende radio.env')
        self.volume = max(0.0, min(1.0, self.volume))
        setup_commands(self)

    async def setup_hook(self):
        await self.tree.sync()
        log.info("Slash-Commands bei Discord synchronisiert")

    def radio_voice(self):
        for voice in self.voice_clients:
            if voice.channel and voice.channel.id == CHANNEL_ID:
                return voice
        return None

    def set_volume(self, value):
        self.volume = max(0.0, min(1.0, round(float(value), 3)))
        voice = self.radio_voice()
        if voice and isinstance(voice.source, discord.PCMVolumeTransformer):
            voice.source.volume = self.volume
        tmp = self.volume_file.with_suffix(".tmp")
        tmp.write_text(json.dumps({"volume": self.volume}), encoding="utf-8")
        tmp.replace(self.volume_file)
        log.info("Radio-Lautstaerke auf %.0f %% geaendert", self.volume * 100)

    async def on_ready(self):
        log.info("Angemeldet als %s (%s)", self.user, self.user.id)
        if self.watchdog_task is None or self.watchdog_task.done():
            self.watchdog_task = asyncio.create_task(self.watchdog())
        if self.nowplaying_task is None or self.nowplaying_task.done():
            self.nowplaying_task = asyncio.create_task(run_nowplaying(self, self.resolve_channel))

    async def resolve_channel(self):
        channel = self.get_channel(CHANNEL_ID)
        if channel is None:
            channel = await self.fetch_channel(CHANNEL_ID)
        if not isinstance(channel, discord.VoiceChannel):
            raise RuntimeError("Kanal-ID muss zu einem normalen Discord-Sprachkanal gehoeren")
        return channel

    def after_audio(self, error):
        if error:
            log.error("Audiowiedergabe unterbrochen: %r", error)
        else:
            log.warning("Audio beendet: Watchdog prueft Neustart")

    async def ensure_playing(self):
        async with self.voice_lock:
            channel = await self.resolve_channel()
            voice = discord.utils.get(self.voice_clients, guild=channel.guild)
            if voice is not None and not voice.is_connected():
                try:
                    await voice.disconnect(force=True)
                except Exception:
                    log.exception("Fehler beim Bereinigen der Voice-Verbindung")
                voice = None
            if voice is None:
                log.info("Verbinde mit Kanal %s (%s)", channel.name, CHANNEL_ID)
                voice = await channel.connect(timeout=25, reconnect=True, self_deaf=True)
            elif voice.channel.id != CHANNEL_ID:
                await voice.move_to(channel)
            if self.paused_by_user:
                if voice.is_playing():
                    voice.pause()
                return
            if voice.is_paused():
                voice.resume()
            if not voice.is_playing():
                source = discord.FFmpegPCMAudio(
                    STREAM_URL,
                    before_options=(
                        "-reconnect 1 -reconnect_streamed 1 "
                        "-reconnect_delay_max 5 -rw_timeout 15000000"
                    ),
                    options="-vn -loglevel error",
                )
                voice.play(discord.PCMVolumeTransformer(source, volume=self.volume), after=self.after_audio)
                log.info("AzuraCast-Stream gestartet")

    async def watchdog(self):
        await self.wait_until_ready()
        while not self.is_closed():
            try:
                await self.ensure_playing()
            except asyncio.CancelledError:
                raise
            except Exception:
                log.exception("Voice/Stream-Problem: neuer Versuch in 15 Sekunden")
            await asyncio.sleep(15)


RadioBot().run(TOKEN, reconnect=True, log_handler=None)
