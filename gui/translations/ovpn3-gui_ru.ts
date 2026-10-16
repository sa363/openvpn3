<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru_RU">
    <context>
        <name>AuthDialog</name>
        <message>
            <source>%1 - %2</source>
            <translation>%1 - %2</translation>
        </message>
        <message>
            <source>Sign in</source>
            <translation>Вход</translation>
        </message>
        <message>
            <source>Response:</source>
            <translation>Ответ:</translation>
        </message>
        <message>
            <source>Username:</source>
            <translation>Имя пользователя:</translation>
        </message>
        <message>
            <source>Password:</source>
            <translation>Пароль:</translation>
        </message>
        <message>
            <source>Remember username</source>
            <translation>Запомнить имя пользователя</translation>
        </message>
        <message>
            <source>Remember password</source>
            <translation>Запомнить пароль</translation>
        </message>
    </context>
    <context>
        <name>Connection</name>
        <message>
            <source>Worker did not stop in time, terminating it</source>
            <translation>Рабочий процесс не остановился вовремя, принудительное завершение</translation>
        </message>
        <message>
            <source>Management interface connected</source>
            <translation>Подключено к интерфейсу управления</translation>
        </message>
        <message>
            <source>Cannot connect to the management interface: %1</source>
            <translation>Не удаётся подключиться к интерфейсу управления: %1</translation>
        </message>
        <message>
            <source>Management interface disconnected</source>
            <translation>Интерфейс управления отключён</translation>
        </message>
        <message>
            <source>%1 authentication failed</source>
            <translation>Ошибка аутентификации %1</translation>
        </message>
        <message>
            <source>Authentication failed for %1</source>
            <translation>Ошибка аутентификации для %1</translation>
        </message>
        <message>
            <source>FATAL: %1</source>
            <translation>КРИТИЧЕСКАЯ ОШИБКА: %1</translation>
        </message>
        <message>
            <source>NOTIFY: %1</source>
            <translation>УВЕДОМЛЕНИЕ: %1</translation>
        </message>
        <message>
            <source>Management error: %1</source>
            <translation>Ошибка управления: %1</translation>
        </message>
        <message>
            <source>Disconnected</source>
            <translation>Отключено</translation>
        </message>
        <message>
            <source>Connecting</source>
            <translation>Подключение</translation>
        </message>
        <message>
            <source>Connected</source>
            <translation>Подключено</translation>
        </message>
        <message>
            <source>Reconnecting</source>
            <translation>Переподключение</translation>
        </message>
        <message>
            <source>Disconnecting</source>
            <translation>Отключение</translation>
        </message>
        <message>
            <source>Failed</source>
            <translation>Ошибка</translation>
        </message>
        <message>
            <source>Worker not found: %1</source>
            <translation>Рабочий процесс не найден: %1</translation>
        </message>
        <message>
            <source>The ovpnagent service is not running. Start it from an elevated prompt with "ovpnagent.exe install" or "ovpnagent.exe run", or select omicli.exe in Settings and run the GUI as administrator.</source>
            <translation>Служба ovpnagent не запущена. Запустите её из командной строки администратора командой "ovpnagent.exe install" или "ovpnagent.exe run", либо выберите omicli.exe в настройках и запустите GUI от имени администратора.</translation>
        </message>
        <message>
            <source>ovpnagent service is not running</source>
            <translation>служба ovpnagent не запущена</translation>
        </message>
        <message>
            <source>No free port for the management interface</source>
            <translation>Нет свободного порта для интерфейса управления</translation>
        </message>
        <message>
            <source>Cannot start worker: %1</source>
            <translation>Не удаётся запустить рабочий процесс: %1</translation>
        </message>
        <message>
            <source>Starting %1</source>
            <translation>Запуск %1</translation>
        </message>
        <message>
            <source>Starting worker</source>
            <translation>Запуск рабочего процесса</translation>
        </message>
        <message>
            <source>Credentials entry cancelled</source>
            <translation>Ввод учётных данных отменён</translation>
        </message>
        <message>
            <source>Paused</source>
            <translation>Приостановлено</translation>
        </message>
        <message>
            <source>Worker exited (code %1)</source>
            <translation>Рабочий процесс завершён (код %1)</translation>
        </message>
        <message>
            <source>Worker crashed</source>
            <translation>Рабочий процесс аварийно завершился</translation>
        </message>
        <message>
            <source>Worker exited unexpectedly</source>
            <translation>Рабочий процесс неожиданно завершился</translation>
        </message>
        <message>
            <source>%1: %2</source>
            <translation>%1: %2</translation>
        </message>
        <message>
            <source>Another connection already redirects all traffic (redirect-gateway). Only one connection can be the default route; disconnect the other one first.</source>
            <translation>Весь трафик уже перенаправляет другое подключение (redirect-gateway). Маршрутом по умолчанию может быть только одно подключение; сначала отключите другое.</translation>
        </message>
        <message>
            <source>Using HTTP proxy %1:%2</source>
            <translation>Используется HTTP-прокси %1:%2</translation>
        </message>
        <message>
            <source>No free VPN adapter for this connection: all adapters of the needed driver (ovpn-dco, TAP or Wintun) are in use or none is installed. Each simultaneous connection needs its own adapter; add one from an elevated prompt, e.g. with "tapctl create --hwid ovpn-dco" from OpenVPN Community. The worker log shows which driver was used.</source>
            <translation>Нет свободного VPN-адаптера для этого подключения: все адаптеры нужного драйвера (ovpn-dco, TAP или Wintun) заняты или не установлены. Каждому одновременному подключению нужен свой адаптер; добавьте его из командной строки администратора, например командой "tapctl create --hwid ovpn-dco" из OpenVPN Community. Какой драйвер использовался, видно в журнале рабочего процесса.</translation>
        </message>
    </context>
    <context>
        <name>ConnectionManager</name>
        <message>
            <source>A profile named "%1" already exists</source>
            <translation>Профиль с именем «%1» уже существует</translation>
        </message>
        <message>
            <source>Cannot copy %1 to %2</source>
            <translation>Не удаётся скопировать %1 в %2</translation>
        </message>
    </context>
    <context>
        <name>ConnectionsWindow</name>
        <message>
            <source>Connections</source>
            <translation>Подключения</translation>
        </message>
        <message>
            <source>Profile</source>
            <translation>Профиль</translation>
        </message>
        <message>
            <source>State</source>
            <translation>Состояние</translation>
        </message>
        <message>
            <source>VPN address</source>
            <translation>VPN-адрес</translation>
        </message>
        <message>
            <source>Traffic in / out</source>
            <translation>Трафик вх. / исх.</translation>
        </message>
        <message>
            <source>Connect</source>
            <translation>Подключить</translation>
        </message>
        <message>
            <source>Disconnect</source>
            <translation>Отключить</translation>
        </message>
        <message>
            <source>Status</source>
            <translation>Статус</translation>
        </message>
        <message>
            <source>Import...</source>
            <translation>Импорт...</translation>
        </message>
        <message>
            <source>Open folder</source>
            <translation>Открыть папку</translation>
        </message>
        <message>
            <source>Settings...</source>
            <translation>Настройки...</translation>
        </message>
        <message>
            <source>Connect on startup</source>
            <translation>Подключать при запуске</translation>
        </message>
        <message>
            <source>Forget saved password</source>
            <translation>Забыть сохранённый пароль</translation>
        </message>
    </context>
    <context>
        <name>QObject</name>
        <message>
            <source>No system tray available.</source>
            <translation>Область уведомлений недоступна.</translation>
        </message>
        <message>
            <source>Browse...</source>
            <translation>Обзор...</translation>
        </message>
    </context>
    <context>
        <name>SettingsDialog</name>
        <message>
            <source>Settings</source>
            <translation>Настройки</translation>
        </message>
        <message>
            <source>Profiles folder:</source>
            <translation>Папка профилей:</translation>
        </message>
        <message>
            <source>Profiles folder</source>
            <translation>Папка профилей</translation>
        </message>
        <message>
            <source>Worker executable:</source>
            <translation>Рабочий процесс:</translation>
        </message>
        <message>
            <source>Worker executable</source>
            <translation>Рабочий процесс</translation>
        </message>
        <message>
            <source>Programs (*.exe)</source>
            <translation>Программы (*.exe)</translation>
        </message>
        <message>
            <source>omicliagent.exe sets up tunnels through the ovpnagent service and needs no administrator rights. omicli.exe works without the service but the GUI must then run as administrator.</source>
            <translation>omicliagent.exe настраивает туннели через службу ovpnagent и не требует прав администратора. omicli.exe работает без службы, но тогда GUI нужно запускать от имени администратора.</translation>
        </message>
        <message>
            <source>Connect through an HTTP proxy</source>
            <translation>Подключаться через HTTP-прокси</translation>
        </message>
        <message>
            <source>proxy.example.com</source>
            <translation>proxy.example.com</translation>
        </message>
        <message>
            <source>Port:</source>
            <translation>Порт:</translation>
        </message>
        <message>
            <source>Proxy:</source>
            <translation>Прокси:</translation>
        </message>
        <message>
            <source>Used for all connections instead of the http-proxy of the profiles. Proxy credentials are asked for when the proxy needs them. Connections through a proxy don't use ovpn-dco-win.</source>
            <translation>Используется для всех подключений вместо http-proxy из профилей. Логин и пароль прокси запрашиваются, если прокси их требует. Подключения через прокси не используют ovpn-dco-win.</translation>
        </message>
        <message>
            <source>Enter the proxy host name or address.</source>
            <translation>Укажите имя или адрес прокси-сервера.</translation>
        </message>
    </context>
    <context>
        <name>StatusWindow</name>
        <message>
            <source>State:</source>
            <translation>Состояние:</translation>
        </message>
        <message>
            <source>VPN address:</source>
            <translation>VPN-адрес:</translation>
        </message>
        <message>
            <source>Server:</source>
            <translation>Сервер:</translation>
        </message>
        <message>
            <source>Traffic:</source>
            <translation>Трафик:</translation>
        </message>
        <message>
            <source>Connected for:</source>
            <translation>Время подключения:</translation>
        </message>
        <message>
            <source>Connect</source>
            <translation>Подключить</translation>
        </message>
        <message>
            <source>Disconnect</source>
            <translation>Отключить</translation>
        </message>
        <message>
            <source>Copy log</source>
            <translation>Копировать журнал</translation>
        </message>
        <message>
            <source>Edit profile</source>
            <translation>Изменить профиль</translation>
        </message>
        <message>
            <source>Hide</source>
            <translation>Скрыть</translation>
        </message>
        <message>
            <source>in %1, out %2</source>
            <translation>вх. %1, исх. %2</translation>
        </message>
        <message>
            <source>Log files</source>
            <translation>Файлы журнала</translation>
        </message>
        <message>
            <source>Open the folder with the logs of this and the earlier sessions</source>
            <translation>Открыть папку с журналами этого и предыдущих сеансов</translation>
        </message>
    </context>
    <context>
        <name>TrafficGraph</name>
        <message>
            <source>in %1</source>
            <translation>вх. %1</translation>
        </message>
        <message>
            <source>out %1</source>
            <translation>исх. %1</translation>
        </message>
        <message>
            <source>%1, last 5 min</source>
            <translation>%1, последние 5 мин</translation>
        </message>
    </context>
    <context>
        <name>TrayController</name>
        <message>
            <source>Default gateway conflict</source>
            <translation>Конфликт шлюза по умолчанию</translation>
        </message>
        <message>
            <source>Both "%1" and the active connection "%2" redirect all traffic (redirect-gateway). Only one of them can be the default route.</source>
            <translation>И «%1», и активное подключение «%2» перенаправляют весь трафик (redirect-gateway). Маршрутом по умолчанию может быть только одно из них.</translation>
        </message>
        <message>
            <source>Disconnect "%1"</source>
            <translation>Отключить «%1»</translation>
        </message>
        <message>
            <source>Import profile</source>
            <translation>Импорт профиля</translation>
        </message>
        <message>
            <source>OpenVPN profiles (*.ovpn *.conf);;All files (*)</source>
            <translation>Профили OpenVPN (*.ovpn *.conf);;Все файлы (*)</translation>
        </message>
        <message>
            <source>Profile imported</source>
            <translation>Профиль импортирован</translation>
        </message>
        <message>
            <source>No profiles</source>
            <translation>Нет профилей</translation>
        </message>
        <message>
            <source>Connect</source>
            <translation>Подключить</translation>
        </message>
        <message>
            <source>Disconnect</source>
            <translation>Отключить</translation>
        </message>
        <message>
            <source>Show status</source>
            <translation>Показать статус</translation>
        </message>
        <message>
            <source>Connections...</source>
            <translation>Подключения...</translation>
        </message>
        <message>
            <source>Disconnect all</source>
            <translation>Отключить все</translation>
        </message>
        <message>
            <source>Import profile...</source>
            <translation>Импорт профиля...</translation>
        </message>
        <message>
            <source>Reload profiles</source>
            <translation>Обновить список профилей</translation>
        </message>
        <message>
            <source>Settings...</source>
            <translation>Настройки...</translation>
        </message>
        <message>
            <source>Exit</source>
            <translation>Выход</translation>
        </message>
        <message>
            <source>OpenVPN3</source>
            <translation>OpenVPN3</translation>
        </message>
        <message>
            <source>Not connected</source>
            <translation>Нет подключений</translation>
        </message>
        <message>
            <source>The server asked to open an unsupported URL:
%1</source>
            <translation>Сервер запросил открытие неподдерживаемого адреса:
%1</translation>
        </message>
        <message>
            <source>The server requires sign-in in a web browser:
%1

Open it now?</source>
            <translation>Сервер требует входа через браузер:
%1

Открыть сейчас?</translation>
        </message>
        <message>
            <source>%1 is connected</source>
            <translation>%1 подключено</translation>
        </message>
        <message numerus="yes">
            <source>%n connection(s) are active. Disconnect and exit?</source>
            <translation>
                <numerusform>Активно %n подключение. Отключить и выйти?</numerusform>
                <numerusform>Активно %n подключения. Отключить и выйти?</numerusform>
                <numerusform>Активно %n подключений. Отключить и выйти?</numerusform>
            </translation>
        </message>
        <message>
            <source>%1 is connected, address %2</source>
            <translation>%1 подключено, адрес %2</translation>
        </message>
    </context>
</TS>
