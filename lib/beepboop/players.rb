module Beepboop
  module Players
    def self.register(information)
      @players ||= []
      @players << {
        :name        => information[:name]        || "Unnamed",
        :extension   => information[:extension]   || "bin",
        :description => information[:description] || "",
        :classname   => information[:classname]   || "NilClass"
      }
    end

    def self.players
      @players.map do |player|
        player[:class] = Beepboop::Players.const_get(player[:classname].intern)
        player
      end
    end
  end
end