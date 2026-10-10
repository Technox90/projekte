"""BBQ-Chaos-Deutschland: 24/7 Discord radio stream relay."""
import asyncio
import logging
import os
import discord

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger("bbq-radio")

TOKEN = os.environ["DISCORD_BOT_TOKEN"]
CHANNEL_ID = int(os.environ["DISCORD_CHANNEL_ID"])
STREAM_URL = os.environ["RADIO_STREAM_URL"]


class RadioBot(discord.Client):
    def __init__(self):
        super().__init__(intents=discord.Intents.none())
        self.watchdog_task = None
        self.voice_lock = asyncio.Lock()

    async def on_ready(self):
        log.info("Angemeldet als %s (%s)", self.user, self.user.id)
        if self.watchdog_task is None or self.watchdog_task.done():
            self.watchdog_task = asyncio.create_task(self.watchdog())

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
            if not voice.is_playing():
                source = discord.FFmpegPCMAudio(
                    STREAM_URL,
                    before_options=(
                        "-reconnect 1 -reconnect_streamed 1 "
                        "-reconnect_delay_max 5 -rw_timeout 15000000"
                    ),
                    options="-vn -loglevel error",
                )
                voice.play(source, after=self.after_audio)
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
