{ pkgs, config, lib, ... }:
let cfg = config.services.tetra-decoder;
in {
  options.services.tetra-decoder = with lib; {
    enable = mkOption {
      type = types.bool;
      default = false;
      description = ''
        Wheather to enable tetra-decoder receiver.
      '';
    };
    user = mkOption {
      type = types.str;
      default = "tetra-decoder";
      description = "systemd user";
    };
    group = mkOption {
      type = types.str;
      default = "tetra-decoder";
      description = "group of systemd user";
    };
    instances = mkOption {
      type = types.attrsOf (types.submodule {
        options.receivePort = mkOption {
          type = types.port;
          default = 42000;
          description = ''
            Port where the decoder receives its bits on.
          '';
        };
        options.transmitPort = mkOption {
          type = types.port;
          default = 42100;
          description = ''
            Port where the decoder transmits its data on.
          '';
        };
        options.extraDecoderArgs = mkOption {
          type = types.str;
          default = "";
          description = ''
            Extra arguments for tetra-decoder decoder
          '';
        };
        options.borzoiUrl = mkOption {
          type = types.str;
          default = "";
          description = ''
            Url for HTTP Post to borzoi
          '';
        };
        options.borzoiStationUuid = mkOption {
          type = types.str;
          default = "00000000-0000-0000-0000-000000000000";
          description = ''
            UUID of the borzoi station
          '';
        };
      });
      default = { };
      description = ''
        Instances with names of the tetra-decoder receiver.
      '';
    };
  };

  config = lib.mkIf cfg.enable {
    systemd.services = lib.concatMapAttrs (instanceName: instanceConfig: {
      "tetra-decoder-${instanceName}" = {
        enable = true;
        wantedBy = [ "multi-user.target" ];

        script = ''
          exec ${pkgs.tetra-decoder}/bin/tetra-decoder --rx ${
            toString instanceConfig.receivePort
          } --tx ${toString instanceConfig.transmitPort} ${instanceConfig.extraDecoderArgs} &
        '';

        serviceConfig = {
          Type = "forking";
          User = cfg.user;
          Restart = "on-failure";
          StartLimitBurst = "2";
          StartLimitIntervalSec = "150s";
        };
      };

      "tetra-sender-${instanceName}" = {
        enable = true;
        wantedBy = [ "multi-user.target" ];

        script = let
          pythonScript = pkgs.writeScript "tetra-sender-${instanceName}" ''
            #!${pkgs.python3.withPackages(ps: [ ps.requests ])}/bin/python3
            # -*- coding: utf-8 -*-

            import json
            import socket
            import requests
            import sys

            UDP_IP = "127.0.0.1"
            UDP_PORT = ${toString instanceConfig.transmitPort}
            BORZOI_URL = "${toString instanceConfig.borzoiUrl}"
            BORZOI_STATION_UUID = "${toString instanceConfig.borzoiStationUuid}"

            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.bind((UDP_IP, UDP_PORT))

            old_data = ""
            while True:
              data, addr = sock.recvfrom(4096)

              print(f"received message: {data}")
              sys.stdout.flush()

              old_data += data.decode('utf-8')

              if len(old_data) > 0 and old_data[-1] != "\n":
                continue
              else:
                try:
                  data = json.loads(old_data.strip('\n'))
                  old_data = ""
                except:
                  print(f"Error parsing json: {old_data}")
                  old_data = ""
                  continue

              try:
                data['source_ssi'] = data['from']['ssi']
                data['destination_ssi'] = data['to']['ssi']
                data['telegram_type'] = data['type']
                del data['from']
                del data['to']
                del data['type']
                data['arbitrary'] = {"bits_in_last_byte": data["bits_in_last_byte"]}
                del data["bits_in_last_byte"]
                data["station"] = BORZOI_STATION_UUID
                data["time"] = data["time"].replace('_', 'T')
              except:
                print(f"Could not send data to borzoi: {data}")
                continue

              try:
                r = requests.post(BORZOI_URL, json=data)
              except requests.exceptions.ConnectionError:
                print(f"Borzoi refused connection")
                continue

              if r.status_code != 200:
                print(f"Send following data to borzoi: {data}")
                print(f"Sending data to borzoi failed with HTTP code: {r.status_code}")
          '';
        in ''
          exec ${pythonScript} &
        '';

        serviceConfig = {
          Type = "forking";
          User = cfg.user;
          Restart = "on-failure";
          StartLimitBurst = "2";
          StartLimitIntervalSec = "150s";
        };
      };
    }) cfg.instances;

    users.groups."${cfg.group}" = { };

    # user accounts for systemd units
    users.users."${cfg.user}" = {
      name = "${cfg.user}";
      description = "This users runs tetra-decoder decoder";
      isSystemUser = true;
      homeMode = "770";
      group = cfg.group;
    };
  };
}
